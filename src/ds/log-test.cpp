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
#include "log.h"
#include "catch.hpp"

TEST_CASE("dsRequire", "[dsLog][ds]") {
    auto&& x = 5;
    DS_REQUIRE(x == 5);
    DS_REQUIRE(x == 5, "should not happen");
    DS_REQUIRE(x == 5, "should not happen", x);
    try {
        DS_REQUIRE(x == 4, x);
        CHECK(false);
    } catch (const std::exception &e) {
        // TODO(someday): introduce macro that checks substring in exception message
        auto&& what = std::string(e.what());
        INFO(what);
        CHECK(what.find("x == 4") != std::string::npos);
        CHECK(what.find("x:5") != std::string::npos);
    }
}

TEST_CASE("dsAssert", "[dsLog][ds]") {
    // make sure the code compiles
    DS_ASSERT(true);
    DS_ASSERT(true, "hi");
}
