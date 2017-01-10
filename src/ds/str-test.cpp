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
#include "catch.hpp"

namespace ds {

namespace {
enum class Enum { A, B };
std::ostream& operator<<(std::ostream& stream, Enum x) {
    switch (x) {
      case Enum::A: return stream << "a";
      case Enum::B: return stream << "b";
    }
  }
}

TEST_CASE("dsStrSmoke", "[dsStr][ds]") {
    CHECK(ds::str() == "");
    CHECK(ds::str("a") == "a");
    CHECK(ds::str("a", "b") == "ab");
    CHECK(ds::str("a", 4, "b") == "a4b");
    CHECK(ds::str("a", -4, "b") == "a-4b");
    CHECK(ds::str("-", Enum::A, Enum::B, "-") == "-ab-");
}

} // namespace ds
