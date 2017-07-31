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
#pragma once

#include <ds/asio/io-service.h>
#include <ds/asio/timer.h>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/chrono.hpp>
#include <boost/signals2.hpp>
#include <random>

namespace ds {
namespace vdsm {

// TODO find better place for this function
/**
 * /Poor men's std::experimental::randint
 */
inline int randInt() {
    // gcc 4.5 does not support thread_local modifier.
    // static is fine, as this whole code base is designed to be single threaded
    static std::minstd_rand generator; // WARN: constant seed
    static std::uniform_int_distribution<int> distribution;
    return distribution(generator);
}

/**
 * Socket that automatically reconnects to given remote address until it succeeds.
 *
 * It is safe to delete the object at any time.
 * Handlers queued in io_service that captured this may be invoked
 * after delete but they do nothing on error operation_aborted.
 *
 * TODO: exponential backoff? Immediate first reconnect?
 */
class ReconnectingSocket {
public:
    ReconnectingSocket(ds::asio::IoService& ioService);
    ~ReconnectingSocket();

    void setResolveTimeout(boost::chrono::milliseconds x) { m_resolveTimeout = x; }
    boost::chrono::milliseconds resolveTimeout() const { return m_resolveTimeout; }

    void setConnectTimeout(boost::chrono::milliseconds x) { m_connectTimeout = x; }
    boost::chrono::milliseconds connectTimeout() const { return m_connectTimeout; }

    /** Idle time before new connection is initialed. */
    void setIdleTimeout(boost::chrono::milliseconds x) { m_idleTimeout = x; }
    boost::chrono::milliseconds idleTimeout() const { return m_idleTimeout; }

    boost::asio::ip::tcp::socket& socket() { return m_socket; }

    /**
     * Connection was opened.
     *
     * Reading / writing code is responsible to call \ref reset()
     * to start new connection attempt.
     * Typically it does it in response to an error.
     */
    boost::signals2::signal<void()>& connected() { return m_connected; }

    /**
     * Opened connection was closed.
     *
     * This signal is emitted exactly once for each emitted connected signal,
     * but not if the object is destroyed.
     *
     * Reading / writing code is responsible to call \ref reset()
     * to start new connection attempt.
     * Typically it does it in response to an error.
     */
    boost::signals2::signal<void()>& disconnected() { return m_disconnected; }

    /**
     * Set resolver query. Query is executed before each new connection.
     *
     * TODO(someday): This method asserts when called after calling \ref start().
     * Is shall just disconnect from old connection and start maintaining
     * connection to new address.
     * Maybe also remove \ref start() and \ref isActive() methods.
     */
    void setQuery(const boost::asio::ip::tcp::resolver::query& query);

    /**
     * Start maintaining active connection.
     *
     * Call \ref setQuery before \ref start().
     */
    void start();

    bool isActive() const { return m_isActive; }

    /**
     * Close connection if it is open and retry new connect after recovery timeout.
     */
    void reset();

    enum class State {
        disconnected,
        connecting,
        connected,
    };
    State state() const { return m_state; }

private:
    boost::asio::ip::tcp::socket m_socket;
    bool m_isActive;
    boost::asio::ip::tcp::resolver m_resolver;
    boost::asio::ip::tcp::resolver::query m_query;
    ds::asio::Abortable m_abortable;

    // Iterator points to endpoint used in next reconnect attempt.
    // We resolve query again if it points to end
    boost::asio::ip::tcp::resolver::iterator m_resolvedQueryIt;
    // TODO use dummy mutexes in signal class to save some CPU
    boost::signals2::signal<void()> m_connected;
    boost::signals2::signal<void()> m_disconnected;

    State m_state;
    ds::asio::Timer m_timer;

    // TODO Force explicit timeout configuration in child classes?
    boost::chrono::milliseconds m_resolveTimeout;
    boost::chrono::milliseconds m_connectTimeout;
    boost::chrono::milliseconds m_idleTimeout;

    /** Reconnect whithout another call within timeout. */
    void asyncWaitTimer(boost::chrono::milliseconds timeout);
    void resetImpl();
    void close();
    void asyncRecover();
    void asyncResolve(ds::asio::Abortable::Handle abortableHandle);
    void asyncConnect(ds::asio::Abortable::Handle abortableHandle);
};
}
} // namespace ds::vdsm
