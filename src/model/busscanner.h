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

#ifndef BUSSCANNER_H
#define BUSSCANNER_H

#include "src/logger.h"
#include "src/ds485types.h"
#include "src/businterface.h"

namespace dss {

  class StructureQueryBusInterface;
  class Apartment;
  class DSMeter;
  class ModelMaintenance;
  class Zone;

  class BusScanner {
  public:
    BusScanner(StructureQueryBusInterface& _interface, Apartment& _apartment, ModelMaintenance& _maintenance);
    bool scanDSMeter(boost::shared_ptr<DSMeter> _dsMeter);
    bool scanDeviceOnBus(boost::shared_ptr<DSMeter> _dsMeter, boost::shared_ptr<Zone> _zone, devid_t _shortAddress);
    bool scanDeviceOnBus(boost::shared_ptr<DSMeter> _dsMeter, devid_t _shortAddress);
    void syncBinaryInputStates(boost::shared_ptr<DSMeter> _dsMeter);
  private:
    bool scanZone(boost::shared_ptr<DSMeter> _dsMeter, boost::shared_ptr<Zone> _zone);
    bool scanGroupsOfZone(boost::shared_ptr<DSMeter> _dsMeter, boost::shared_ptr<Zone> _zone);
    bool scanStatusOfZone(boost::shared_ptr<DSMeter> _dsMeter, boost::shared_ptr<Zone> _zone);
    void log(const std::string& _line, aLogSeverity _severity = lsDebug);
    bool initializeDeviceFromSpec(boost::shared_ptr<DSMeter> _dsMeter, boost::shared_ptr<Zone> _zone, DeviceSpec_t& _spec);
    void scheduleOEMReadout(const boost::shared_ptr<Device> _pDevice);
  private:
    Apartment& m_Apartment;
    StructureQueryBusInterface& m_Interface;
    ModelMaintenance& m_Maintenance;
  };

} // namespace dss

#endif // BUSSCANNER_H
