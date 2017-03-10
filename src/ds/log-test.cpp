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

TEST_CASE("DS_REQUIRE", TAGS) {
    int x = 5;

    SECTION("Success does not throw") {
        DS_REQUIRE(x == 5);
        DS_REQUIRE(x == 5, "should not happen");
        DS_REQUIRE(x == 5, "should not happen", x);
    }

    SECTION("Failure throws") {
        int line = 0;
        auto f = [&] {
            line = __LINE__ + 1;
            DS_REQUIRE(x == 4, "Variable x must be 4.", x);
        };
        CHECK_THROWS_FIND(f(), ds::str("Variable x must be 4. x:5 condition:x == 4 file:ds/log-test.cpp:", line));
    }
}

TEST_CASE("DS_FAIL_REQUIRE", TAGS) {
    int x = 5;
    SECTION("throws") {
        int line = 0;
        auto f = [&] {
            line = __LINE__ + 1;
            DS_FAIL_REQUIRE("Invalid x.", x);
        };
        CHECK_THROWS_FIND(f(), ds::str("Invalid x. x:5 file:ds/log-test.cpp:", line));
    }
}

TEST_CASE("DS_ASSERT", TAGS) {
    SECTION("compiles") {
        DS_ASSERT(true);
        DS_ASSERT(true, "hi");
    }
}

TEST_CASE("DS_FAIL_ASSERT", TAGS) {
    SECTION("compiles") {
        if (0) {
            // TODO(someday): error with g++ (Ubuntu 5.4.0-6ubuntu1~16.04.4) 5.4.0 20160609
            // In file included from ../../build/../src/ds/log-test.cpp:19:0:
            // ../../build/../src/ds/log-test.cpp: In function ‘void ____C_A_T_C_H____T_E_S_T____62()’:
            // ../../build/../src/ds/log.h:75:54: error: expected primary-expression before ‘,’ token
            //  #define DS_LOG_ARG(x) , ::ds::log::trimArg(#x ":"), x, " "
            //                                                       ^
            // ../../build/../src/ds/log.h:36:28: note: in expansion of macro ‘DS_LOG_ARG’
            //  #define DS_MFE_1(_call, x) _call(x)
            //                             ^
            // ../../build/../src/ds/log.h:31:5: note: in expansion of macro ‘DS_MFE_1’
            //      N
            //      ^
            // ../../build/../src/ds/log.h:116:45: note: in expansion of macro ‘DS_MACRO_FOR_EACH’
            //      ::ds::log::_private::assert_(ds::str("" DS_MACRO_FOR_EACH(DS_LOG_ARG, ##__VA_ARGS__), DS_LOG_FILE_CONTEXT))
            //                                              ^
            // ../../build/../src/ds/log-test.cpp:65:13: note: in expansion of macro ‘DS_FAIL_ASSERT’
            //
            // DS_FAIL_ASSERT();

            DS_FAIL_ASSERT("hi");
        }
    }
}

TEST_CASE("dsLogTrimFile", TAGS) {
    SECTION("Do not trim files bellow 32 characters without src/") {
        CHECK(ds::log::_private::trimFile("") == "");
        CHECK(ds::log::_private::trimFile("a") == "a");
        CHECK(ds::log::_private::trimFile("foo.h") == "foo.h");
        CHECK(ds::log::_private::trimFile("foo.cpp") == "foo.cpp");
        CHECK(ds::log::_private::trimFile("1234567890123456789012345678901") == "1234567890123456789012345678901");
        CHECK(ds::log::_private::trimFile("12345678901234567890123456789012") == "12345678901234567890123456789012");
    }

    SECTION("Trim up to last src/") {
        CHECK(ds::log::_private::trimFile("src/foo.cpp") == "foo.cpp");
        CHECK(ds::log::_private::trimFile("../src/foo.cpp") == "foo.cpp");
        CHECK(ds::log::_private::trimFile("src/src/foo.cpp") == "foo.cpp");
        CHECK(ds::log::_private::trimFile("123456789012345678901234567890123/src/foo.cpp") == "foo.cpp");
    }

    SECTION("Trim with dots to 32 characters") {
        CHECK(ds::log::_private::trimFile("123456789012345678901234567890123") == "...56789012345678901234567890123");
        CHECK(ds::log::_private::trimFile("123456789012345678901234567890.cpp") == "...6789012345678901234567890.cpp");
        CHECK(ds::log::_private::trimFile("src/123456789012345678901234567890.cpp") ==
                "...6789012345678901234567890.cpp");
    }
}
