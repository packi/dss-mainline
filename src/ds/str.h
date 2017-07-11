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

#include <boost/foreach.hpp>
#include <sstream>
#include <vector>
#include "common.h"

namespace ds {

/// Concatenate all arguments into string using std::ostream operator <<
/// Example: ds:str("int:", 7, " double:", 7.7, " char:", '7'); // == "int:7 double:7.7 char:7"
template <typename... Args>
std::string str(Args&&... args);

// implementation details

namespace _private {

template <typename T>
struct Str {
    template <typename T2>
    static void f(std::ostream& ostream, T2&& x) {
        ostream << std::forward<T2>(x);
    }
};

// uint8_t and int8_t shall be serialized as numbers, not characters
template <>
struct Str<unsigned char> {
    static inline void f(std::ostream& ostream, unsigned char x) { ostream << static_cast<int>(x); }
};

template <>
struct Str<signed char> {
    static inline void f(std::ostream& ostream, signed char x) { ostream << static_cast<int>(x); }
};

template <>
struct Str<std::exception> {
    static inline void f(std::ostream& ostream, const std::exception& x) { ostream << x.what(); }
};

template <typename T>
struct Str<std::vector<T>> {
    static inline void f(std::ostream& ostream, const std::vector<T>& xs) {
        ostream << '(';
        auto first = true;
        BOOST_FOREACH (auto&& x, xs) {
            if (!first) {
                ostream << ',';
            } else {
                first = false;
            }
            Str<typename std::decay<T>::type>::f(ostream, x);
        }
        ostream << ')';
    }
};

inline void strRecursive(std::ostream&) {}
template <typename Arg, typename... Args>
void strRecursive(std::ostream& ostream, Arg&& arg, Args&&... args) {
    Str<typename std::decay<Arg>::type>::f(ostream, std::forward<Arg>(arg));
    strRecursive(ostream, std::forward<Args>(args)...);
}

} // namespace _private

template <typename... Args>
std::string str(Args&&... args) {
    std::ostringstream ostream;
    _private::strRecursive(ostream, std::forward<Args>(args)...);
    return ostream.str();
}

} // namespace ds
