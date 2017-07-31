/*
    Copyright (c) 2016 digitalSTROM AG, Zurich, Switzerland

    This file is part of vdSM.

    vdSM is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 2 of the License.

    vdSM is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with vdSM. If not, see <http://www.gnu.org/licenses/>.
*/
#include "reconnecting-socket.h"

#include <ds/log.h>

typedef boost::system::error_code error_code;
typedef boost::asio::ip::tcp::resolver resolver;

DS_STATIC_LOG_CHANNEL(dsAsioReconnectingSocket);

namespace ds {
namespace vdsm {

ReconnectingSocket::ReconnectingSocket(ds::asio::IoService& ioService)
        : m_socket(ioService),
          m_isActive(false),
          m_resolver(ioService),
          m_query(""),
          m_state(State::disconnected),
          m_timer(ioService),
          m_resolveTimeout(boost::chrono::seconds(20)),
          m_connectTimeout(boost::chrono::seconds(20)),
          m_idleTimeout(boost::chrono::seconds(20)) {
    DS_INFO_ENTER(this);
}

ReconnectingSocket::~ReconnectingSocket() {
    DS_INFO_ENTER(this);
}

void ReconnectingSocket::setQuery(const resolver::query& query) {
    DS_INFO_ENTER(query.host_name(), query.service_name());
    assert(!m_isActive);
    m_query = query;
    m_resolvedQueryIt = resolver::iterator();
}

void ReconnectingSocket::start() {
    DS_INFO_ENTER("");
    DS_ASSERT(!m_isActive);
    m_isActive = true;
    asyncResolve(m_abortable.nextHandle());
}

void ReconnectingSocket::reset() {
    // public method
    DS_INFO_ENTER("");
    resetImpl();
}

void ReconnectingSocket::resetImpl() {
    DS_DEBUG_ENTER(this);
    assert(m_isActive);
    close();
    asyncWaitTimer(m_idleTimeout * (80 + randInt() % 40) / 100);
}

void ReconnectingSocket::close() {
    if (m_state == State::disconnected) {
        return;
    }
    DS_INFO_ENTER((int)m_state);
    auto oldState = m_state;
    m_state = State::disconnected;
    {
        error_code ec; // ignore close errors
        m_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        m_socket.close(ec);
    }
    m_timer.cancel();
    m_abortable.abort();

    if (oldState == State::connected) {
        m_disconnected(); // notify higher code
    }
}

void ReconnectingSocket::asyncWaitTimer(boost::chrono::milliseconds timeout) {
    DS_DEBUG_ENTER(timeout.count());
    assert(m_isActive);

    m_timer.expiresFromNow(timeout, [this]() mutable {
        DS_INFO_ENTER("");

        close();
        // Try next endpoint or resolve endpoints again
        auto abortableHandle = m_abortable.nextHandle();
        if (m_resolvedQueryIt != resolver::iterator()) {
            asyncConnect(std::move(abortableHandle));
        } else {
            asyncResolve(std::move(abortableHandle));
        }
    });
}

void ReconnectingSocket::asyncResolve(ds::asio::Abortable::Handle abortableHandle) {
    DS_DEBUG_ENTER(m_query.host_name());
    m_resolver.async_resolve(m_query, [this, abortableHandle](const error_code& e, resolver::iterator it) mutable {
        if (abortableHandle.isAborted() || e == boost::asio::error::operation_aborted) {
            DS_DEBUG_LEAVE("");
            return;
        }
        DS_DEBUG_ENTER(e.message());
        if (e) {
            DS_WARNING("Failed to resolve.", m_query.host_name());
            resetImpl();
            return;
        }
        m_resolvedQueryIt = it;
        asyncConnect(std::move(abortableHandle));
    });

    asyncWaitTimer(m_resolveTimeout);
}

void ReconnectingSocket::asyncConnect(ds::asio::Abortable::Handle abortableHandle) {
    DS_DEBUG_ENTER("");

    if (m_resolvedQueryIt == resolver::iterator()) {
        resetImpl();
        return;
    }

    m_state = State::connecting;
    auto endpoint = m_resolvedQueryIt->endpoint();
    m_resolvedQueryIt++;
    m_socket.async_connect(endpoint, [this, abortableHandle, endpoint](const error_code& e) mutable {
        if (abortableHandle.isAborted() || e == boost::asio::error::operation_aborted) {
            return; //'this' may already be deleted
        }
        DS_DEBUG_ENTER(e.message());
        if (e) {
            DS_WARNING("Failed to connect.", endpoint.address().to_string(), endpoint.port(), e.message());
            // Preceed to next endpoint immediately.
            // Some endpoints can be unaccessible
            // and it should not trigger idle timeout
            m_socket.close();
            asyncConnect(std::move(abortableHandle));
            return;
        }
        m_timer.cancel();
        m_state = State::connected;

        DS_NOTICE("Connected.", endpoint.address().to_string(), endpoint.port());
        m_connected(); // notify higher code
    });

    asyncWaitTimer(m_connectTimeout);
}
}
} // namespace ds::vdsm
