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
#include <ds/catch/catch.h>

static const char* TAGS = "[dsLog][ds]";

TEST_CASE("dsRequire", TAGS) {
    auto&& x = 5;
    DS_REQUIRE(x == 5);
    DS_REQUIRE(x == 5, "should not happen");
    DS_REQUIRE(x == 5, "should not happen", x);

    CHECK_THROWS_FIND(DS_REQUIRE(x == 4, x), "x == 4");
    CHECK_THROWS_FIND(DS_REQUIRE(x == 4, x), "x:5");
}

TEST_CASE("dsAssert", TAGS) {
    // make sure the code compiles
    DS_ASSERT(true);
    DS_ASSERT(true, "hi");
}
