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

#ifndef DS485BUSREQUESTDISPATCHER_H
#define DS485BUSREQUESTDISPATCHER_H

#include "core/busrequestdispatcher.h"

namespace dss {

  class DS485Interface;
  
  class DS485BusRequestDispatcher : public BusRequestDispatcher {
  public:
    virtual void dispatchRequest(boost::shared_ptr<BusRequest> _pRequest);
    void setProxy(DS485Interface* _pInterface) {
      m_pInterface = _pInterface;
    }
  private:
    DS485Interface* m_pInterface;
  }; // DS485BusRequestDispatcher

} // namespace dss

#endif // DS485BUSREQUESTDISPATCHER_H
