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
#include "random.h"
#include "catch.hpp"

namespace ds {

TEST_CASE("dsRandintSmoke", "[dsRandint][dsRandom][ds]") {
    CHECK(ds::randint(0,0) == 0);
    CHECK(ds::randint(1,1) == 1);
    CHECK(ds::randint(2,2) == 2);
    for (int i = 0; i < 10; i++) {
      auto x = ds::randint(7,10);
      CHECK(x >= 7);
      CHECK(x <= 10);
    }
}

} // namespace ds
