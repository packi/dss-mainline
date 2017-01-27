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
#include "common.h"
#include <ds/catch/catch.h>

static const char* TAGS = "[dsCommon][ds]";

namespace {
class Base {
public:
    virtual void virtualFoo();
};

class Child : public Base {
    virtual void virtualFoo() DS_OVERRIDE;
};

TEST_CASE("DS_STATIC_ASSERT", TAGS) {
    DS_STATIC_ASSERT(true, "");
    DS_STATIC_ASSERT(std::is_base_of<Base DS_COMMA Child>::value, "`Base` must be base of `Child`");
}

TEST_CASE("DS_NULLPTR", TAGS) {
    CHECK((void *) 0 == DS_NULLPTR);
}

} // namespace
