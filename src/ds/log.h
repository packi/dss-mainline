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

#include <stdexcept>
#include "str.h"

/// Provide a for-each construct for variadic macros.
/// Source https://codecraft.co/2014/11/25/variadic-macros-tricks/

// Accept any number of args >= N, but expand to just the Nth one.
// Here, N == 22.
#define DS_GET_NTH_ARG(                                                                                         \
        _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, N, ...) \
    N

// Define some macros to help us create overrides based on the
// arity of a for-each-style macro.
#define DS_MFE_0(_call, ...)
#define DS_MFE_1(_call, x) _call(x)
#define DS_MFE_2(_call, x, ...) _call(x) DS_MFE_1(_call, __VA_ARGS__)
#define DS_MFE_3(_call, x, ...) _call(x) DS_MFE_2(_call, __VA_ARGS__)
#define DS_MFE_4(_call, x, ...) _call(x) DS_MFE_3(_call, __VA_ARGS__)
#define DS_MFE_5(_call, x, ...) _call(x) DS_MFE_4(_call, __VA_ARGS__)
#define DS_MFE_6(_call, x, ...) _call(x) DS_MFE_5(_call, __VA_ARGS__)
#define DS_MFE_7(_call, x, ...) _call(x) DS_MFE_6(_call, __VA_ARGS__)
#define DS_MFE_8(_call, x, ...) _call(x) DS_MFE_7(_call, __VA_ARGS__)
#define DS_MFE_9(_call, x, ...) _call(x) DS_MFE_8(_call, __VA_ARGS__)
#define DS_MFE_10(_call, x, ...) _call(x) DS_MFE_9(_call, __VA_ARGS__)
#define DS_MFE_11(_call, x, ...) _call(x) DS_MFE_10(_call, __VA_ARGS__)
#define DS_MFE_12(_call, x, ...) _call(x) DS_MFE_11(_call, __VA_ARGS__)
#define DS_MFE_13(_call, x, ...) _call(x) DS_MFE_12(_call, __VA_ARGS__)
#define DS_MFE_14(_call, x, ...) _call(x) DS_MFE_13(_call, __VA_ARGS__)
#define DS_MFE_15(_call, x, ...) _call(x) DS_MFE_14(_call, __VA_ARGS__)
#define DS_MFE_16(_call, x, ...) _call(x) DS_MFE_15(_call, __VA_ARGS__)
#define DS_MFE_17(_call, x, ...) _call(x) DS_MFE_16(_call, __VA_ARGS__)
#define DS_MFE_18(_call, x, ...) _call(x) DS_MFE_17(_call, __VA_ARGS__)
#define DS_MFE_19(_call, x, ...) _call(x) DS_MFE_18(_call, __VA_ARGS__)
#define DS_MFE_20(_call, x, ...) _call(x) DS_MFE_19(_call, __VA_ARGS__)

#define DS_MACRO_FOR_EACH(x, ...)                                                                              \
    DS_GET_NTH_ARG("ignored", ##__VA_ARGS__, DS_MFE_20, DS_MFE_19, DS_MFE_18, DS_MFE_17, DS_MFE_16, DS_MFE_15, \
            DS_MFE_14, DS_MFE_13, DS_MFE_12, DS_MFE_11, DS_MFE_10, DS_MFE_9, DS_MFE_8, DS_MFE_7, DS_MFE_6,     \
            DS_MFE_5, DS_MFE_4, DS_MFE_3, DS_MFE_2, DS_MFE_1, DS_MFE_0) (x, ##__VA_ARGS__)

namespace ds {
namespace log {

/// Return passed stringified argument if it is worth to log or an empty string othwerwise.
///
/// Example:
/// trimArg(##hello) -> "hello"
/// trimArg(##"hello") -> ""
constexpr const char *trimArg(const char *x) {
    return (x[0] != '"') ? x : "";
}

#define DS_LOG_ARG(x) , " ", ::ds::log::trimArg(#x ":"), x

// * `DS_REQUIRE(condition)`:  Check external input and preconditions
// e.g. to validate parameters passed from a caller.
// A failure indicates that the caller is buggy.
// Variadic arguments are attached to the message like in debug macros.
// Example:
//
// DS_REQUIRE(value >= 0, "Value cannot be negative.", value);
//
#define DS_REQUIRE(condition, ...) \
    if (DS_LIKELY(condition)) {} else \
        throw std::runtime_error(ds::str(__FILE__, ':', __LINE__, '/', __FUNCTION__, ' ', #condition \
            DS_MACRO_FOR_EACH(DS_LOG_ARG, ##__VA_ARGS__)))

// Assert the `condition` is true.
// This macro should be used to check for bugs in the surrounding code (broken invariant, ...)
// and its dependencies, but NOT to check for invalid input.
// Example:
//
// DS_ASSERT(m_state == State::CONNECTED, m_state);
//
#define DS_ASSERT(condition, ...) \
    if (DS_LIKELY(condition)) {} else { \
        std::cerr << ds::str(__FILE__, ':', __LINE__, '/', __FUNCTION__, ' ', #condition \
            DS_MACRO_FOR_EACH(DS_LOG_ARG, ##__VA_ARGS__))); \
        abort(); \
    }

} // namespace log
} // namespace ds

