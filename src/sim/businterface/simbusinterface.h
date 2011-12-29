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

#include "src/businterface.h"

#include <boost/shared_ptr.hpp>

namespace dss {

  class DSSim;

  class SimBusInterface : public BusInterface {
  public:
    SimBusInterface(boost::shared_ptr<DSSim> _pSimulation);

    virtual DeviceBusInterface* getDeviceBusInterface() {
      return m_pDeviceBusInterface.get();
    }
    virtual StructureQueryBusInterface* getStructureQueryBusInterface() {
      return m_pStructureQueryBusInterface.get();
    }
    virtual MeteringBusInterface* getMeteringBusInterface() {
      return m_pMeteringBusInterface.get();
    }
    virtual StructureModifyingBusInterface* getStructureModifyingBusInterface() {
      return m_pStructureModifyingBusInterface.get();
    }
    virtual ActionRequestInterface* getActionRequestInterface() {
      return m_pActionRequestInterface.get();
    }

    virtual void setBusEventSink(BusEventSink* _eventSink);
  private:
    boost::shared_ptr<DSSim> m_pSimulation;
    boost::shared_ptr<DeviceBusInterface> m_pDeviceBusInterface;
    boost::shared_ptr<StructureQueryBusInterface> m_pStructureQueryBusInterface;
    boost::shared_ptr<MeteringBusInterface> m_pMeteringBusInterface;
    boost::shared_ptr<StructureModifyingBusInterface> m_pStructureModifyingBusInterface;
    boost::shared_ptr<ActionRequestInterface> m_pActionRequestInterface;
  };


} // namespace dss
