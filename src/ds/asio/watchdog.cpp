/*
    Copyright (c) 2015 digitalSTROM AG, Zurich, Switzerland

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
#include "watchdog.h"
#include <ds/log.h>
#include <atomic>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/future.hpp>
#include <thread>

DS_STATIC_LOG_CHANNEL(dsAsioWatchdog);

namespace ds {
namespace asio {

struct Watchdog::Impl {
    boost::asio::io_service &m_ioService;
    boost::chrono::microseconds m_failureLatency;
    volatile bool m_shutdown;
    boost::mutex m_mutex;
    boost::condition_variable m_conditionVariable;
    std::thread m_thread;
    std::function<void()> m_timeoutFunction;

    Impl(boost::asio::io_service &ioService, boost::chrono::microseconds failureLatency)
            : m_ioService(ioService),
              m_failureLatency(failureLatency),
              m_shutdown(false),
              m_thread([this]() { run(); }),
              m_timeoutFunction([this]() { defaultTimeoutFunction(); }) {
        auto failureLatencyMs = boost::chrono::duration_cast<boost::chrono::milliseconds>(m_failureLatency).count();
        DS_INFO_ENTER(failureLatencyMs);
    }
    ~Impl() {
        DS_INFO_ENTER("");
        m_shutdown = true;
        m_conditionVariable.notify_one();
        m_thread.join();
    }

    void run() {
        DS_INFO_ENTER("");
        boost::unique_lock<boost::mutex> lock(m_mutex);
        boost::chrono::steady_clock clock;
        while (true) {
            auto timePoint = clock.now();
            std::atomic_bool pong(false);
            DS_INFO("Watchdog ping");
            m_ioService.dispatch([&]() {
                DS_INFO("Watchdog pong");
                pong = true;
            });
            m_conditionVariable.wait_for(lock, m_failureLatency, [&]() {
                auto duration = clock.now() - timePoint;
                auto durationMs = boost::chrono::duration_cast<boost::chrono::milliseconds>(duration).count();
                DS_INFO("Watchdog wait for ", m_shutdown, durationMs);
                return m_shutdown || (duration >= m_failureLatency);
            });
            if (m_shutdown) {
                break;
            }
            if (!pong) {
                DS_INFO("Watchdog timeout");
                m_timeoutFunction();
            }
        }
    }

    void defaultTimeoutFunction() {
        DS_ERROR("Watchdog failure");
        abort();
    }
};

Watchdog::Watchdog(boost::asio::io_service &ioService, boost::chrono::microseconds failureLatency)
        : m_impl(new Impl(ioService, failureLatency)) {}

Watchdog::~Watchdog() = default;

void Watchdog::setTimeoutFunction(std::function<void()> &&x) {
    m_impl->m_timeoutFunction = std::move(x);
}

} // namespace sqlite3
} // namespace ds
