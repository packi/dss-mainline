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
#include "log.h"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include "common.h"

namespace ds {
namespace log {
namespace _private {

static std::string trimFileToLastSrc(std::string file) {
    auto src = "src/";
    auto srcLen = ::strlen(src);
    auto pos = file.rfind(src);
    if (pos != std::string::npos) {
        file.erase(0, pos + srcLen);
    }
    return file;
}

static std::string trimFileWithDotsToMaxSize(std::string file) {
    constexpr auto maxSize = 32;
    constexpr auto dotsSize = 3;
    if (file.size() <= maxSize) {
        return file;
    }
    file.erase(0, file.size() - maxSize);
    DS_ASSERT(file.size() == maxSize);

    for (int i = 0; i < dotsSize; i++) {
        file[i] = '.';
    }
    return file;
}

std::string trimFile(std::string file) {
    return trimFileWithDotsToMaxSize(trimFileToLastSrc(std::move(file)));
}

void assert_(const std::string& x) {
    std::cerr << x;
    abort();
}

} // namespace _private

} // namespace log
} // namespace ds
