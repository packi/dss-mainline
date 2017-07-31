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
#include "enum.h"
#include <ds/catch/catch.h>
#include <ds/str.h>
#include <array>

static const char* TAGS = "[dsEnum][ds]";

namespace {

enum class Transport { CAR, PLANE };

std::array<std::pair<Transport, const char*>, 2> dsEnumOptions(Transport) {
    return std::array<std::pair<Transport, const char*>, 2>{{
            {Transport::CAR, "car"}, {Transport::PLANE, "plane"},
    }};
}

std::ostream& operator<<(std::ostream& stream, Transport x) {
    return ds::ostream(stream, x);
}

TEST_CASE("For enum with dsEnumOptions", TAGS) {
    SECTION("tryFromEnum<const char*> returns matching string") {
        CHECK(ds::tryFromEnum<const char*>(Transport::CAR).value() == std::string("car"));
        CHECK(ds::tryFromEnum<const char*>(Transport::PLANE).value() == std::string("plane"));
    }

    SECTION("tryFromEnum<const char*> returns none on non-matching option") {
        CHECK(bool(ds::tryFromEnum<const char*>(static_cast<Transport>(100))) == false);
    }

    SECTION("tryFromEnum<std::string> returns matching string") {
        CHECK(ds::tryFromEnum<std::string>(Transport::CAR).value() == "car");
        CHECK(ds::tryFromEnum<std::string>(Transport::PLANE).value() == "plane");
    }

    SECTION("tryFromEnum<std::string> returns none on non-matching option") {
        CHECK(bool(ds::tryFromEnum<std::string>(static_cast<Transport>(100))) == false);
    }

    SECTION("tryToEnum returns matching value") {
        CHECK(ds::tryToEnum<Transport>("car").value() == Transport::CAR);
        CHECK(ds::tryToEnum<Transport>("plane").value() == Transport::PLANE);
    }

    SECTION("tryToEnum returns none on non-matching option") {
        CHECK(bool(ds::tryToEnum<Transport>("notAMatchingOption")) == false);
    }

    SECTION("tryToEnum accepts std::string") {
        CHECK(ds::tryToEnum<Transport>(std::string("car")).value() == Transport::CAR);
    }

    SECTION("ostream << writes matching values as string(value)") {
        CHECK(ds::str(Transport::CAR) == "car(0)");
        CHECK(ds::str(Transport::PLANE) == "plane(1)");
    }

    SECTION("isValidEnum == true for matching value ") { CHECK(ds::isValidEnum(Transport::CAR)); }

    SECTION("isValidEnum == false for non-matching value ") { CHECK(!ds::isValidEnum(static_cast<Transport>(100))); }

    SECTION("ostream << writes non-matching value as unknown(value)") {
        CHECK(ds::str(static_cast<Transport>(100)) == "unknown(100)");
    }
}

} // namespace
