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

#include <boost/asio/steady_timer.hpp>
#include <boost/chrono/chrono.hpp>

namespace ds {
namespace asio {

/// Asio (steady) timer class.
///
/// Nearly all code wants to use steady clock, so `steady` is skipped from the name.
///
/// Unit tests will need an option to control the time programatically.
/// It will be much easier to deploy this change if all code uses this class.
class Timer : public ::boost::asio::basic_waitable_timer<boost::chrono::steady_clock> {
public:
    typedef duration Duration;
    Timer(boost::asio::io_service& ioService)
        : ::boost::asio::basic_waitable_timer<boost::chrono::steady_clock>(ioService) {
    }
    /// Expires randomly in closed interval [a, b] relatively to now.
    void randomlyExpiresFromNow(Duration a, Duration b);

    /// Expires randomly `p` percent around `d` relatively to now.
    void randomlyExpiresFromNowPercent(Duration d, int p);
};

} // namespace asio
} // namespace ds
