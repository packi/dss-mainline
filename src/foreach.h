/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland

    Author: Patrick Staehlin, futureLAB AG <pstaehlin@futurelab.ch>

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

#ifndef FOREACH_H_
#define FOREACH_H_

#include <boost/foreach.hpp>

namespace boost
{
    // Suggested work-around for https://svn.boost.org/trac/boost/ticket/6131
    namespace BOOST_FOREACH = foreach;
}

#define foreach BOOST_FOREACH
#define foreach_r BOOST_REVERSE_FOREACH

#endif /* FOREACH_H_ */
