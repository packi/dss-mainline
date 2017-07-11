/*
    Copyright (c) 2016 digitalSTROM.org, Zurich, Switzerland

    This file is part of digitalSTROM Server.

    digitalSTROM Server is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    digitalSTROM Server is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with digitalSTROM Server. If not, see <http://www.gnu.org/licenses/>.
*/
#include "timer.h"
#include <ds/asio/catch.h>

static const char* TAGS = "[dsAsioTimer][dsAsio][ds]";

TEST_CASE("dsAsioTimer", TAGS) {
    ds::asio::catch_::IoService ioService;
    ds::asio::Timer timer(ioService);
    auto&& LATENCY = ds::asio::catch_::LATENCY;

    SECTION("is started") {
        bool asyncWaitCalled = false;
        timer.expiresFromNow(boost::chrono::milliseconds(0), [&] {
            asyncWaitCalled = true;
            ioService.stop();
        });

        SECTION("and callback is called") {
            CHECK_STOP_RUN(ioService);
            CHECK(asyncWaitCalled == true);
        }

        SECTION("callback is NOT called after cancel") {
            timer.cancel();
            CHECK_NO_STOP_RUN(ioService);
            CHECK(asyncWaitCalled == false);
        }
    }

    SECTION("expiresFromNow") {
        timer.expiresFromNow(boost::chrono::milliseconds(0), [&] { ioService.stop(); });
        CHECK_STOP_RUN(ioService);
    }

    SECTION("expiresNow") {
        timer.expiresNow([&] { ioService.stop(); });
        CHECK_STOP_RUN(ioService);
    }

    SECTION("randomlyExpiresFromNow") {
        timer.randomlyExpiresFromNow(LATENCY * 3 / 4, LATENCY, [&] { ioService.stop(); });
        CHECK_NO_STOP_RUN_FOR(ioService, LATENCY / 2);
        CHECK_STOP_RUN_FOR(ioService, LATENCY);
    }

    SECTION("randomlyExpiresFromNowPercentDown") {
        timer.randomlyExpiresFromNowPercentDown(LATENCY, 25, [&] { ioService.stop(); });
        CHECK_NO_STOP_RUN_FOR(ioService, LATENCY / 2);
        CHECK_STOP_RUN_FOR(ioService, LATENCY);
    }

    SECTION("canceled timer does not call callback") {
        // Once the asio timer posts the callback to ioService,
        // the callback will be executed as posted regardless of
        // whether the timer is canceled or destroyed in mean time.
        ioService.post([&] { ioService.stop(); });

        SECTION("callback is scheduled and timer is canceled") {
            timer.expiresNow([&] { CHECK(false); });
            CHECK_STOP_RUN(ioService);
            timer.cancel();
        }

        SECTION("callback is scheduled and timer is destroyed") {
            ds::asio::Timer timer2(ioService);
            timer2.expiresNow([&] { CHECK(false); });
            CHECK_STOP_RUN(ioService);
        }
        CHECK_NO_STOP_RUN(ioService);
    }
}
