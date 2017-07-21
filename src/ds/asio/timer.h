/*
 *  Copyright (c) 2016 digitalSTROM.org, Zurich, Switzerland
 *
 *  This file is part of digitalSTROM Server.
 *
 *  digitalSTROM Server is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  digitalSTROM Server is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with digitalSTROM Server. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <ds/common.h>
#include <boost/asio/steady_timer.hpp>
#include <boost/chrono/chrono.hpp>
#include "abortable.h"
#include "io-service.h"

namespace ds {
namespace asio {

/// Asio (steady) timer class.
///
/// Nearly all code wants to use steady clock, so `steady` is skipped from the name.
///
/// Unit tests will need an option to control the time programatically.
/// It will be much easier to deploy this change if all code uses this class.
class Timer : private ::boost::asio::basic_waitable_timer<boost::chrono::steady_clock> {
public:
    typedef ::boost::asio::basic_waitable_timer<boost::chrono::steady_clock> Super;
    typedef duration Duration;
    Timer(IoService &ioService) : Super(ioService) {}

    /// Convenient method calling `expires_from_now(x)` and `asyncWait(f)`.
    template <typename F>
    void expiresFromNow(Duration d, F &&f) {
        expires_from_now(d);
        asyncWait(std::forward<F>(f));
    }

    /// Convenient method calling `expiresFromNow(0, f)`.
    template <typename F>
    void expiresNow(F &&f) {
        expiresFromNow(Duration(0), std::forward<F>(f));
    }

    /// Expires randomly in closed interval [a, b] relatively to now.
    template <typename F>
    void randomlyExpiresFromNow(Duration a, Duration b, F &&f) {
        randomlyExpiresFromNow(a, b);
        asyncWait(std::forward<F>(f));
    }

    /// Expires randomly in closed interval [d - d * p / 100, d] relatively to now.
    template <typename F>
    void randomlyExpiresFromNowPercentDown(Duration d, int p, F &&f) {
        randomlyExpiresFromNowPercentDown(d, p);
        asyncWait(std::forward<F>(f));
    }

    /// Calls async_wait, but calls callback only on NO error.
    ///
    /// Callback can be sure that it is not called when timer is destroyed / cancelled.
    template <typename F>
    void asyncWait(F &&f) {
        static_assert(std::is_void<decltype(f())>::value, "required F type: void ()");
        auto abortableHandle = m_abortable.nextHandle();
        // TODO(c++14): move capture f, inline isDestroed
        async_wait([f, abortableHandle](boost::system::error_code e) mutable {
            if (abortableHandle.isAborted() || e) {
                return; // timer was aborted
            }
            f();
        });
    }

    void cancel() {
        Super::cancel();
        m_abortable.abort();
    }

private:
    Abortable m_abortable;

    void randomlyExpiresFromNow(Duration a, Duration b);
    void randomlyExpiresFromNowPercentDown(Duration d, int p);
};

} // namespace asio
} // namespace ds
