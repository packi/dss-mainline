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
#include <boost/optional.hpp>
#include <cstring>
#include <ostream>
#include "common.h"

namespace ds {

// Introduces reflection for enums and handy template conversion functions
// using this reflection.
//
// Example use:
//
// 1. define the enum
//
//   enum class Transport { CAR, PLANE };
//
// 2. provide enum to string mapping
//
//   std::array<std::pair<Transport, const char*>, 2> dsEnumOptions(Transport) {
//     return {{
//       {Transport::CAR, "car"},
//       {Transport::PLANE, "plane"},
//     }};
//   };
//   inline std::ostream& operator<<(std::ostream& stream, Transport x) { return ds::ostream(stream, x); }
//
// NOTE: dsEnumOptions and ostream << operator must be declared
// in the same namespace as enum to let it just work everywhere.
//
// The `dsEnumOptions` argument value is ignored,
// the argument only triggers C++ argument dependent lookup.
//
// 3. use
//
//   ds::tryFromEnum<const char*>(Transport:CAR)
//   ds::tryFromEnum<std::string>(Transport:CAR)
//   ds::tryFromEnum<int>(Transport:CAR)
//
//   ds::tryToEnum<Transport>("car")
//   ds::tryToEnum<Transport>(1)
//
//   ds::isValidEnum(Transport::CAR)
//
//   std::cout << Transport:CAR
//

template <typename T, typename = decltype(dsEnumOptions(T()))>
DS_CONSTEXPR boost::optional<T> tryToEnum(const char* x) {
    BOOST_FOREACH (auto&& option, dsEnumOptions(T())) {
        if (strcmp(x, option.second) == 0) {
            return option.first;
        }
    }
    return boost::none;
}

template <typename T, typename = decltype(dsEnumOptions(T()))>
DS_CONSTEXPR boost::optional<T> tryToEnum(const std::string& x) {
    return tryToEnum<T>(x.c_str());
}

template <typename T, typename = decltype(dsEnumOptions(T()))>
DS_CONSTEXPR boost::optional<T> tryToEnum(int x) {
    BOOST_FOREACH (auto&& option, dsEnumOptions(T())) {
        if (static_cast<int>(option.first) == x) {
            return option.first;
        }
    }
    return boost::none;
}

namespace private_ {
template <typename R, typename T, typename = decltype(dsEnumOptions(T()))>
struct FromEnum {
    static boost::optional<R> f(T) { return boost::none; }
};
template <typename T>
struct FromEnum<const char*, T, decltype(dsEnumOptions(T()))> {
    static boost::optional<const char*> f(T x) {
        BOOST_FOREACH (auto&& option, dsEnumOptions(T())) {
            if (option.first == x) {
                return option.second;
            }
        }
        return boost::none;
    }
};
template <typename T>
struct FromEnum<std::string, T, decltype(dsEnumOptions(T()))> {
    static boost::optional<std::string> f(T x) {
        if (auto s = FromEnum<const char*, T, decltype(dsEnumOptions(T()))>::f(x)) {
            return std::string(*s);
        }
        return boost::none;
    }
};
template <typename T>
struct FromEnum<int, T, decltype(dsEnumOptions(T()))> {
    static boost::optional<int> f(T x) {
        BOOST_FOREACH (auto&& option, dsEnumOptions(T())) {
            if (option.first == x) {
                return static_cast<int>(x);
            }
        }
        return boost::none;
    }
};
} // namespace private_

template <typename R, typename T, typename = decltype(dsEnumOptions(T()))>
boost::optional<R> tryFromEnum(T x) {
    return private_::FromEnum<R, T, decltype(dsEnumOptions(T()))>::f(x);
}

template <typename T, typename = decltype(dsEnumOptions(T()))>
bool isValidEnum(T x) {
    return bool(private_::FromEnum<const char*, T, decltype(dsEnumOptions(T()))>::f(x));
}

template <typename T, typename = decltype(dsEnumOptions(T()))>
std::ostream& ostream(std::ostream& stream, T x) {
    return [&]() -> std::ostream& {
        if (auto&& s = tryFromEnum<const char*>(x)) {
            return stream << *s;
        } else {
            return stream << "unknown";
        }
    }() << '('
        << static_cast<int>(x) << ')';
}

} // namespace ds
