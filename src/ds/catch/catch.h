// Copyright (c) 2017 digitalSTROM AG, Zurich, Switzerland
//
// This file is part of digitalSTROM Server.
// vdSM is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, version 2 of the License.
//
// vdSM is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with digitalSTROM Server. If not, see <http://www.gnu.org/licenses/>.

#pragma once

/// TODO(someday): do we want to define CATCH_CONFIG_PREFIX_ALL here?

#include "catch.hpp"

/// require expression to throw and exception containing given message
#define REQUIRE_THROWS_FIND(expression, message) REQUIRE_THROWS_WITH(expression, \
        ::Catch::Matchers::Contains(message))
/// check expression to throw and exception containing given message
#define CHECK_THROWS_FIND(expression, message) CHECK_THROWS_WITH(expression, ::Catch::Matchers::Contains(message))
