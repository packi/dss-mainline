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
#pragma once

#include <random>

namespace ds {

namespace _private {
/// Get thread local random engine initially seeded to random value.
std::default_random_engine& getRandomEngine();
} // _private

/// Generates a random integer in the closed interval [a, b].
/// Poor man's std::experimental::randint
template <class IntType>
IntType randint(IntType a, IntType b) {
    return std::uniform_int_distribution<IntType>(a, b)(_private::getRandomEngine());
}

} // namespace ds
