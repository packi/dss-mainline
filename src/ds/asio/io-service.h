// Copyright (c) 2017 digitalSTROM AG, Zurich, Switzerland
//
// This file is part of digitalSTROM Server.
// vdSM is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, version 2 of the License.
//
// vdSM is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with digitalSTROM Server. If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <thread>
#include <boost/asio/io_service.hpp>

namespace ds {
namespace asio {

/// Asio io_service specialized to run in single thread
/// to act as an event loop.
///
/// It requires the thread that calls the construct to
/// call also the run methods.
class IoService : public boost::asio::io_service {
public:
  IoService();
  ~IoService();

  std::thread::id getThreadId() const { return m_threadId; }
  bool thisThreadMatches() const { return m_threadId == std::this_thread::get_id(); }

  /// DS_REQUIRE(thisThreadMatches()) before asio::io_service::run()
  std::size_t run();

private:
  std::thread::id m_threadId;
};

} // namespace asio
} // namespace ds
