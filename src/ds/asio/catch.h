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
#include <ds/common.h>

namespace ds {
namespace asio {
namespace catch_ {

/// ds::asio::IoService specialized for use in catch unit tests.
///
/// It has always work to do
class IoService : public ::ds::asio::IoService {
public:
    IoService() = default;
    ~IoService() = default;

    enum RunResult { STOPPED, EXPIRED };

    /// Run io_service till stopped up up for \ref duration.
    /// @return true when run stopped before
    RunResult runFor(boost::chrono::milliseconds duration);

private:
    /// Hide because it is not appropriate to call `run` in tests
    std::size_t run() { return ds::asio::IoService::run(); }
    std::size_t run_one() { return ds::asio::IoService::run_one(); }
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

#define INTERNAL_STOP_RUN_FOR(ioService, duration, macro)                                                  \
    do {                                                                                                   \
        static_assert(std::is_base_of<::ds::asio::catch_::IoService DS_COMMA decltype(ioService)>::value,  \
                "ioService must be if ::ds::asio::catch_::IoService type");                                \
        auto dsAsioCatchDuration = (duration);                                                             \
        INFO("Running even loop for " << dsAsioCatchDuration.count() << "ms and expect stop");             \
        macro(ioService.runFor(dsAsioCatchDuration) == ::ds::asio::catch_::IoService::RunResult::STOPPED); \
    } while (0)

/// Run event loop and require stop
#define REQUIRE_STOP_RUN(ioService) INTERNAL_STOP_RUN_FOR(ioService, ::ds::asio::catch_::TIMEOUT / 2, REQUIRE)

/// Run event loop and check stop
#define CHECK_STOP_RUN(ioService) INTERNAL_STOP_RUN_FOR(ioService, ::ds::asio::catch_::TIMEOUT / 2, CHECK)

/// Run event loop for duration and require stop
#define REQUIRE_STOP_RUN_FOR(ioService, duration) INTERNAL_STOP_RUN_FOR(ioService, duration, REQUIRE)

/// Run event loop for duration and check stop
#define CHECK_STOP_RUN_FOR(ioService, duration) INTERNAL_STOP_RUN_FOR(ioService, duration, CHECK)

#define INTERNAL_NO_STOP_RUN_FOR(ioService, duration, macro)                                               \
    do {                                                                                                   \
        static_assert(std::is_base_of<::ds::asio::catch_::IoService DS_COMMA decltype(ioService)>::value,  \
                "ioService must be if ::ds::asio::catch_::IoService type");                                \
        auto dsAsioCatchDuration = (duration);                                                             \
        INFO("Running even loop for " << dsAsioCatchDuration.count() << "ms and expect NO stop");          \
        macro(ioService.runFor(dsAsioCatchDuration) == ::ds::asio::catch_::IoService::RunResult::EXPIRED); \
    } while (0)

/// Run event loop and require NO stop
#define REQUIRE_NO_STOP_RUN(ioService) INTERNAL_NO_STOP_RUN_FOR(ioService, ::ds::asio::catch_::LATENCY * 2, REQUIRE)

/// Run event loop and check NO stop
#define CHECK_NO_STOP_RUN(ioService) INTERNAL_NO_STOP_RUN_FOR(ioService, ::ds::asio::catch_::LATENCY * 2, CHECK)

/// Run event loop for duration and require NO stop
#define REQUIRE_NO_STOP_RUN_FOR(ioService, duration) INTERNAL_NO_STOP_RUN_FOR(ioService, duration, REQUIRE)

/// Run event loop for duration and check NO stop
#define CHECK_NO_STOP_RUN_FOR(ioService, duration) INTERNAL_NO_STOP_RUN_FOR(ioService, duration, CHECK)

// implementation details

// Can be moved to .cpp, if more code accumulates here. This file is header only for now.
inline IoService::RunResult IoService::runFor(boost::chrono::milliseconds duration) {
    ds::asio::Timer timer(*this);
    RunResult result = RunResult::STOPPED;

    // workaround for gcc4.5
    // ../src/ds/asio/catch.h: In lambda function:
    // ../src/ds/asio/catch.h:109:12: error: cannot call member function 'void boost::asio::io_service::stop()' without
    // object
    auto this_ = this;

    timer.expiresFromNow(duration, [&] {
        result = RunResult::EXPIRED;
        this_->stop();
    });

    reset();
    run();
    return result;
}

} // namespace catch_
} // namespace asio
} // namespace ds
