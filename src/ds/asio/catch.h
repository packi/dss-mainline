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

#include <ds/asio/io-service.h>
#include <ds/asio/timer.h>
#include <ds/catch/catch.h>

namespace ds {
namespace asio {
namespace catch_ {

/// ds::asio::IoService specialized for use in catch unit tests
class IoService : public ::ds::asio::IoService {
public:
  IoService() {}
  ~IoService() {}

  enum RunResult { STOPPED, EXPIRED };

  /// Run io_service till stopped up up for \ref duration.
  /// @return true when run stopped before
  RunResult run(boost::chrono::milliseconds duration);

private:
  /// Hide because it is not appropriate to call `run` in tests
  std::size_t run() { return ds::asio::IoService::run(); }
};

/**
 * Duration for timers that are not expected to fire during test.
 * The whole test is expected to finish before this duration expires.
 */
static constexpr boost::chrono::milliseconds TIMEOUT = boost::chrono::milliseconds(2000);

/**
 * Duration for timers that are expected to fire during test.
 */
static constexpr boost::chrono::milliseconds LATENCY = boost::chrono::milliseconds(12);


#define INTERNAL_RUN_STOP(ioService, macro) \
        do { \
            ::boost::chrono::milliseconds dsAsioCatchDuration = ::ds::asio::catch_::TIMEOUT / 2; \
            INFO("Running even loop for " << dsAsioCatchDuration.count() << "ms and expect stop"); \
            macro(ioService.run(dsAsioCatchDuration) == ::ds::asio::catch_::IoService::RunResult::STOPPED); \
        } while (0)
#define REQUIRE_RUN_STOP(ioService) INTERNAL_RUN_STOP(ioService, REQUIRE)
#define CHECK_RUN_STOP(ioService) INTERNAL_RUN_STOP(ioService, CHECK)

#define INTERNAL_RUN_NO_STOP(ioService, macro) \
        do { \
            ::boost::chrono::milliseconds dsAsioCatchDuration = ::ds::asio::catch_::LATENCY * 2; \
            INFO("Running even loop for " << dsAsioCatchDuration.count() << "ms and expect NO stop"); \
            macro(ioService.run(dsAsioCatchDuration) == ::ds::asio::catch_::IoService::RunResult::EXPIRED); \
        } while (0)
#define REQUIRE_RUN_NO_STOP(ioService) INTERNAL_RUN_NO_STOP(ioService, REQUIRE)
#define CHECK_RUN_NO_STOP(ioService) INTERNAL_RUN_NO_STOP(ioService, CHECK)

// implementation details

// Can be moved to .cpp, if more code accumulates here. This file is header only for now.
IoService::RunResult IoService::run(boost::chrono::milliseconds duration) {
  ds::asio::Timer timer(*this);
  RunResult result = RunResult::STOPPED;

  timer.expires_from_now(duration);
  timer.asyncWait([&]() {
      result = RunResult::EXPIRED;
      stop();
  });

  reset();
  run();
  return result;
}

} // namespace catch_
} // namespace asio
} // namespace ds
