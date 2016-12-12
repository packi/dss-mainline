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

#include <sstream>

namespace ds {

/// Concatenate all arguments into string using std::ostream operator <<
/// Example: ds:str("int:", 7, " double:", 7.7, " char:", '7'); // == "int:7 double:7.7 char:7"
template <typename... Args>
std::string str(Args&&... args);

// implementation details

namespace _private {

inline void strRecursive(std::ostream& ostream) {}
template <typename Arg, typename... Args>
void strRecursive(std::ostream& ostream, Arg&& arg, Args&&... args) {
    ostream << std::forward<Arg>(arg);
    strRecursive(ostream, std::forward<Args>(args)...);
}

} // namespace _private

template <typename... Args>
std::string str(Args&&... args) {
    std::ostringstream ostream;
    _private::strRecursive(ostream, std::forward<Args>(args)...);
    return ostream.str();
}
}
