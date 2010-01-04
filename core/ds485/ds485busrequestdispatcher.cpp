/*
    Copyright (c) 2010 digitalSTROM.org, Zurich, Switzerland

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

#include "ds485busrequestdispatcher.h"
#include "core/model/busrequest.h"
#include "core/ds485/ds485.h"
#include "core/foreach.h"
#include "core/ds485const.h"
#include "core/DS485Interface.h"

namespace dss {

  void DS485BusRequestDispatcher::dispatchRequest(boost::shared_ptr<BusRequest> _pRequest) {
    boost::shared_ptr<PacketBuilderHintsBase> hints = _pRequest->getBuilderHints();
    boost::shared_ptr<DS485CommandFrame> frame(new DS485CommandFrame());
    frame->getHeader().setBroadcast(hints->isBroadcast());
    frame->getHeader().setDestination(hints->getTarget());
    frame->setCommand(CommandRequest);
    frame->getPayload().add<uint8_t>(hints->getFunctionID());
    if(hints->getNumberAddressParameter() == 2) {
      frame->getPayload().add<uint16_t>(hints->getAddressParameter(0));
      frame->getPayload().add<uint16_t>(hints->getAddressParameter(1));
    } else {
      frame->getPayload().add<uint16_t>(hints->getAddressParameter(0));
    }
    int numParam = hints->getNumberOfParameter();
    for(int iParam = 0; iParam < numParam; iParam++) {
      frame->getPayload().add<uint16_t>(hints->getParameter(iParam));
    }
    m_pInterface->sendFrame(*frame);
  }
} // namespace dss

