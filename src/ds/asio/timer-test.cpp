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

    SECTION("is started") {
        timer.expires_from_now(boost::chrono::milliseconds(0));
        bool asyncWaitCalled = false;
        timer.asyncWait([&] {
            asyncWaitCalled = true;
            ioService.stop();
        });

        SECTION("and callback is called") {
            CHECK_RUN_STOP(ioService);
            CHECK(asyncWaitCalled == true);
        }

        SECTION("callback is NOT called after cancel") {
            timer.cancel();
            CHECK_RUN_NO_STOP(ioService);
            CHECK(asyncWaitCalled == false);
        }
    }
}

