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

#pragma once

#include <boost/asio/io_service.hpp>
#include <boost/chrono/chrono.hpp>
#include <functional>
#include <memory>

namespace ds {
namespace asio {

/// Software watchdog that guards that ioService event loop is not stalled.
///
/// Spawns seperate thread and regurarly pings ioService from it.
/// TODO(systemd): add systemd watchdog support activated by WATCHDOG_USEC
/// variable.
class Watchdog {
public:
    Watchdog(boost::asio::io_service &ioService, boost::chrono::microseconds failureLatency);
    ~Watchdog();

    /// Set function called when watchog times out. Default function calls
    /// `::abort()`.
    ///
    /// WARNING: callback is called from non-ioService thread.
    void setTimeoutFunction(std::function<void()> &&);

private:
    struct Impl;
    const std::unique_ptr<Impl> m_impl;
};
} // namespace asio
} // namespace ds
