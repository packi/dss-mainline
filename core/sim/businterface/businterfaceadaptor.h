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

#ifndef BUSINTERFACEADAPTOR_H_
#define BUSINTERFACEADAPTOR_H_

#include "core/businterface.h"

namespace dss {

  class DSSim;
  class SimBusInterface;

  class BusInterfaceAdaptor : public BusInterface {
    BusInterfaceAdaptor(boost::shared_ptr<BusInterface> _pInner,
                        boost::shared_ptr<DSSim> _pSimulation,
                        boost::shared_ptr<SimBusInterface> _pSimBusInterface);
    virtual DeviceBusInterface* getDeviceBusInterface();
    virtual StructureQueryBusInterface* getStructureQueryBusInterface();
    virtual MeteringBusInterface* getMeteringBusInterface();
    virtual StructureModifyingBusInterface* getStructureModifyingBusInterface();
    virtual ActionRequestInterface* getActionRequestInterface();
  private:
    boost::shared_ptr<BusInterface> m_pInnerBusInterface;
    boost::shared_ptr<DSSim> m_pSimulation;
    boost::shared_ptr<SimBusInterface> m_pSimBusInterface;
    class Implementation;
    boost::shared_ptr<Implementation> m_pImplementation;
  }; // BusInterfaceAdaptor
} // namespace dss

#endif
