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

static const char *TAGS = "[dsCommon][ds]";

namespace {
class Base {
public:
    virtual void virtualFoo();

    static int staticWarnUnusedResult() DS_WARN_UNUSED_RESULT { return 0; }
};

class Child DS_FINAL : public Base {
    virtual void virtualFoo() DS_OVERRIDE;
};

TEST_CASE("DS_NULLPTR", TAGS) {
    CHECK((void *)0 == DS_NULLPTR);
}

DS_NORETURN void noReturn() {
    throw std::runtime_error("error");
}

TEST_CASE("DS_NORETURN", TAGS) {
    CHECK_THROWS(noReturn());
}

TEST_CASE("DS_WARN_UNUSED_RESULT", TAGS) {
    auto x = Base::staticWarnUnusedResult();
}

TEST_CASE("DS_CONCAT", TAGS) {
    SECTION("concatenates two identifiers into one") {
        int ab = 7;
        CHECK(DS_CONCAT(a, b) == 7);
    }
}

TEST_CASE("DS_UNIQUE_NAME", TAGS) {
    SECTION("generates the same identifiers in the same line / macro") {
// Macro to force compilation into one line
#define MACRO                            \
    do {                                 \
        auto DS_UNIQUE_NAME(foo) = 7;    \
        CHECK(DS_UNIQUE_NAME(foo) == 7); \
        DS_UNIQUE_NAME(foo) = 8;         \
        CHECK(DS_UNIQUE_NAME(foo) == 8); \
    } while (0)
        MACRO;
#undef MACRO_CHECK
    }

    SECTION("generates unique identifiers on different lines") {
        auto DS_UNIQUE_NAME(foo) = 7;
        auto DS_UNIQUE_NAME(foo) = 8;
    }
}

namespace {
struct ExpectedException {};
}

TEST_CASE("DS_DEFER", TAGS) {
    SECTION("captures by reference and is called at the end of scope") {
        int calledCount = 0;
        try {
            DS_DEFER(calledCount++);
            throw ExpectedException();
        } catch (const ExpectedException &) {
        }
        CHECK(calledCount == 1);
    }
}

} // namespace
