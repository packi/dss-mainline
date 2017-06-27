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
#include "str.h"
#include <ds/catch/catch.h>

static const char* TAGS = "[dsStr][ds]";

namespace {
enum class Enum { A, B };
std::ostream& operator<<(std::ostream& stream, Enum x) {
    switch (x) {
        case Enum::A:
            return stream << "a";
        case Enum::B:
            return stream << "b";
    }
    return stream << "?";
}
}

TEST_CASE("dsStr", TAGS) {
    SECTION("all arguments are serialized and concatenated") {
        CHECK(ds::str() == "");
        CHECK(ds::str("a") == "a");
        CHECK(ds::str("a", "b") == "ab");
        CHECK(ds::str("a", 4, "b") == "a4b");
        CHECK(ds::str("a", -4, "b") == "a-4b");
    }

    SECTION("ostream operator << is used for serialization") { CHECK(ds::str("-", Enum::A, Enum::B, "-") == "-ab-"); }

    SECTION("unsigned and signed chars are serialized as numbers") {
        CHECK(ds::str(static_cast<unsigned char>('a')) == "97");
        CHECK(ds::str(static_cast<signed char>('a')) == "97");
        CHECK(ds::str(static_cast<uint8_t>('a')) == "97");
        CHECK(ds::str(static_cast<int8_t>('a')) == "97");
    }

    SECTION("chars are serialized as characters") {
        CHECK(ds::str('a') == "a");
        CHECK(ds::str(static_cast<char>('a')) == "a");
    }

    SECTION("std:exceptions references are serialized with what()") {
        auto e = std::runtime_error("a");
        CHECK(ds::str(static_cast<const std::exception&>(e)) == "a");
        CHECK(ds::str(static_cast<std::exception&>(e)) == "a");
    }
}
