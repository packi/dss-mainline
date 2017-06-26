/*
 *  Copyright (c) 2016 digitalSTROM.org, Zurich, Switzerland
 *
 *  This file is part of digitalSTROM Server.
 *
 *  digitalSTROM Server is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  digitalSTROM Server is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with digitalSTROM Server. If not, see <http://www.gnu.org/licenses/>.
 */

#include "timer.h"
#include <ds/log.h>
#include <ds/random.h>

namespace ds {
namespace asio {

void Timer::randomlyExpiresFromNow(Duration a, Duration b) {
    expires_from_now(Duration(ds::randint(a.count(), b.count())));
}

void Timer::randomlyExpiresFromNowPercentDown(Duration d, int p) {
    DS_REQUIRE(p > 0 && p < 100, p);
    randomlyExpiresFromNow(d - d / 100 * p, d);
}

} // namespace asio
} // namespace ds
