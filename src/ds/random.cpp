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
#include <boost/thread/tss.hpp>
#include <random>
#include "common.h"

namespace ds {
namespace _private {

std::default_random_engine& getRandomEngine() {
    // TODO(c++11): use thread_local std::unique_ptr
    static boost::thread_specific_ptr<std::default_random_engine> engine;
    if (!engine.get()) {
        std::random_device device;
        engine.reset(new std::default_random_engine(device()));
    }
    return *engine;
}

} // _private
} // namespace ds
