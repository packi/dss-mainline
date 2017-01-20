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
#include "io-service.h"
#include <thread>
#include <ds/catch/catch.h>

static const char* TAGS = "[dsAsioIoService][dsAsio][ds]";

TEST_CASE("dsAsioIoService", TAGS) {
    ds::asio::IoService ioService;

    SECTION("run works in matching thread") {
        ioService.run();
    }

    SECTION("run throws in non-matching thread") {
        std::thread thread([&]{
            REQUIRE_THROWS_FIND(ioService.run(), "run() is called from wrong thread");
        });
        thread.join();
    }
}
