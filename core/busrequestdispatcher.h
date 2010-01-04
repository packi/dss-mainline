/*
    Copyright (c) 2010 digitalSTROM.org, Zurich, Switzerland
    Copyright (c) 2010 futureLAB AG, Winterthur, Switzerland

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

#ifndef BUSREQUESTDISPATCHER_H
#define BUSREQUESTDISPATCHER_H

#include <boost/shared_ptr.hpp>

namespace dss {

  class BusRequest;

  class BusRequestDispatcher {
  public:
    virtual void dispatchRequest(boost::shared_ptr<BusRequest> _pRequest) = 0;
    virtual ~BusRequestDispatcher() {}; // please the compiler (virtual dtor)
  }; // BusRequestDispatcher

} // namespace dss

#endif // BUSREQUESTDISPATCHER_H
