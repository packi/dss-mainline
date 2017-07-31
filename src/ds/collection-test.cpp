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
#include "collection.h"
#include <ds/catch/catch.h>
#include <ds/str.h>

namespace {

static const char* TAGS = "[dsCollection][ds]";

TEST_CASE("dsCollectionMapValue", TAGS) {
    std::string id = "x";
    auto firstChar = [](const char* x) { return x[0]; };

    SECTION("maps value in Added") {
        auto event = ds::collection::mapValue<const char*>(ds::collection::Added<const char*>(id, "ab"), firstChar);
        auto& added = boost::get<ds::collection::Added<char>>(event);
        CHECK(added.id == id);
        CHECK(added.value == 'a');
    }

    SECTION("preserves Removed") {
        auto event = ds::collection::mapValue<const char*>(ds::collection::Removed<const char*>(id), firstChar);
        auto& removed = boost::get<ds::collection::Removed<char>>(event);
        CHECK(removed.id == id);
    }

    SECTION("maps value in Changed") {
        auto event = ds::collection::mapValue<const char*>(ds::collection::Changed<const char*>(id, "ab"), firstChar);
        auto& changed = boost::get<ds::collection::Changed<char>>(event);
        CHECK(changed.id == id);
        CHECK(changed.value == 'a');
    }

    SECTION("preserves AllForNow") {
        auto event = ds::collection::mapValue<const char*>(ds::collection::AllForNow(), firstChar);
        boost::get<ds::collection::AllForNow>(event);
    }

    SECTION("preserves Reset") {
        auto event = ds::collection::mapValue<const char*>(ds::collection::Reset(), firstChar);
        boost::get<ds::collection::Reset>(event);
    }

    SECTION("compiles for move only type") {
        ds::collection::mapValue<std::unique_ptr<int>>(ds::collection::Reset(), [](std::unique_ptr<int>) { return 0; });
    }
}

TEST_CASE("dsCollectionVector", TAGS) {
    std::string id = "x";
    ds::collection::Vector<int> vector;

    SECTION("add adds value") {
        vector.add(id, 4);
        REQUIRE(vector.size() == 1);
        REQUIRE(vector.front().id == id);
        REQUIRE(vector.front().value == 4);
    }

    SECTION("added event adds value") {
        vector.applyEvent(ds::collection::Added<int>(id, 4));
        REQUIRE(vector.size() == 1);
        REQUIRE(vector.front().id == id);
        REQUIRE(vector.front().value == 4);
    }

    SECTION("add throws on duplicate") {
        vector.add(id, 4);
        CHECK_THROWS(vector.add(id, 4));
    }

    SECTION("remove removes value") {
        vector.add(id, 4);
        vector.remove(id);
        REQUIRE(vector.empty());
    }

    SECTION("remove event removes value") {
        vector.add(id, 4);
        vector.applyEvent(ds::collection::Removed<int>(id));
        REQUIRE(vector.empty());
    }

    SECTION("remove throws on non existing id") { CHECK_THROWS(vector.remove(id)); }

    SECTION("change changes value") {
        vector.add(id, 4);
        vector.change(id, 5);
        REQUIRE(vector.size() == 1);
        REQUIRE(vector.front().id == id);
        REQUIRE(vector.front().value == 5);
    }

    SECTION("change event changes value") {
        vector.add(id, 4);
        vector.applyEvent(ds::collection::Changed<int>(id, 5));
        REQUIRE(vector.size() == 1);
        REQUIRE(vector.front().id == id);
        REQUIRE(vector.front().value == 5);
    }

    SECTION("isAllForNow is initially false") { CHECK(vector.isAllForNow() == false); }

    SECTION("setAllForNow sets isAllForNow") {
        vector.setAllForNow();
        CHECK(vector.isAllForNow() == true);
    }

    SECTION("setAllForNow event sets isAllForNow") {
        vector.applyEvent(ds::collection::AllForNow());
        CHECK(vector.isAllForNow() == true);
    }

    SECTION("reset clears content and isAllForNow") {
        vector.add(id, 4);
        vector.setAllForNow();
        vector.reset();
        REQUIRE(vector.empty());
        CHECK(vector.isAllForNow() == false);
    }

    SECTION("reset event clears content and isAllForNow") {
        vector.add(id, 4);
        vector.setAllForNow();
        vector.applyEvent(ds::collection::Reset());
        REQUIRE(vector.empty());
        CHECK(vector.isAllForNow() == false);
    }

    SECTION("tryAt finds value for an existing entry") {
        vector.add(id, 4);
        CHECK(*vector.tryAt(id) == 4);
    }

    SECTION("tryAt returns nullptr for a non-existing entry") { CHECK(vector.tryAt(id) == DS_NULLPTR); }

    SECTION("at finds value for an existing entry") {
        vector.add(id, 4);
        CHECK(vector.at(id) == 4);
    }

    SECTION("at throws for a non-existing entry") { CHECK_THROWS(vector.at(id)); }
}

} // namespace
