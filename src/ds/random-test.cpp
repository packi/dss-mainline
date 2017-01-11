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
#include <thread>
#include <limits>
#include <catch.hpp>

namespace ds {

static const char* TAGS = "[dsRandint][dsRandom][ds]";

TEST_CASE("dsRandintOneValue", TAGS) {
    CHECK(ds::randint(0, 0) == 0);
    CHECK(ds::randint(1, 1) == 1);
    CHECK(ds::randint(2, 2) == 2);
    CHECK(ds::randint(0, 100) >= 0);
    CHECK(ds::randint(0, 100) <= 100);
    CHECK(ds::randint(1000, 2000) >= 1000);
    CHECK(ds::randint(1000, 2000) <= 2000);
}

TEST_CASE("dsRandintBuckets", TAGS) {
    auto min = 0;
    auto max = 1000000;
    std::vector<int> buckets(10, 0);
    auto bucketsSize = buckets.size();
    auto itemsPerBucket = 50;
    for (int i = 0; i < bucketsSize * itemsPerBucket; i++) {
        auto x = ds::randint(0, max);
        ++buckets.at((x - min) * bucketsSize / (max - min));
    }
    for (int i = 0; i < bucketsSize; i++) {
        CHECK(buckets.at(i) > itemsPerBucket / 4);
    }
}

TEST_CASE("dsRandintThreadRandomSeed", TAGS) {
    auto firstValue = 0LL;
    auto setFirstValueInNewThread = [&](){
        std::thread thread([&](){
            firstValue = ds::randint(std::numeric_limits<long long>::min(), std::numeric_limits<long long>::max());
        });
        thread.join();
    };
    setFirstValueInNewThread();
    CHECK(firstValue != 0);
    auto firstValue1 = firstValue;

    setFirstValueInNewThread();
    CHECK(firstValue != firstValue1);
}


} // namespace ds
