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

#include "simbusinterface.h"

#include "simactionrequestbusinterface.h"
#include "simdevicebusinterface.h"
#include "simmeteringbusinterface.h"
#include "simstructurequerybusinterface.h"

namespace dss {

  //================================================== SimBusInterface

  SimBusInterface::SimBusInterface(boost::shared_ptr<DSSim> _pSimulation)
  : m_pSimulation(_pSimulation)
  {
    m_pActionRequestInterface.reset(new SimActionRequestBusInterface(_pSimulation));
    m_pDeviceBusInterface.reset(new SimDeviceBusInterface(m_pSimulation));
    m_pMeteringBusInterface.reset(new SimMeteringBusInterface(m_pSimulation));
    m_pStructureQueryBusInterface.reset(new SimStructureQueryBusInterface(m_pSimulation));
  }


} // namespace dss