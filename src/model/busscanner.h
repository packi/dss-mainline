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

#include <digitalSTROM/ds.h>
#include "src/logger.h"
#include "src/ds485types.h"
#include "src/businterface.h"
#include "src/ds485/dsbusinterface.h"
#include "model/device.h"
#include "model/set.h"
#include "taskprocessor.h"

namespace dss {

  class StructureQueryBusInterface;
  class DSDeviceBusInterface;
  class Apartment;
  class DSMeter;
  class Device;
  class ModelMaintenance;
  class Zone;
  class Set;

  class BusScanner {
  public:
    BusScanner(StructureQueryBusInterface& _interface, Apartment& _apartment, ModelMaintenance& _maintenance);
    bool scanDSMeter(boost::shared_ptr<DSMeter> _dsMeter);
    bool scanDeviceOnBus(boost::shared_ptr<DSMeter> _dsMeter, boost::shared_ptr<Zone> _zone, devid_t _shortAddress);
    bool scanDeviceOnBus(boost::shared_ptr<DSMeter> _dsMeter, devid_t _shortAddress);
    void syncBinaryInputStates(boost::shared_ptr<DSMeter> _dsMeter, boost::shared_ptr<Device> _device);
  private:
    bool scanZone(boost::shared_ptr<DSMeter> _dsMeter, boost::shared_ptr<Zone> _zone);
    void scanDevicesOfZoneQuick(boost::shared_ptr<DSMeter> _dsMeter, boost::shared_ptr<Zone> _zone);
    bool scanGroupsOfZone(boost::shared_ptr<DSMeter> _dsMeter, boost::shared_ptr<Zone> _zone);
    bool scanStatusOfZone(boost::shared_ptr<DSMeter> _dsMeter, boost::shared_ptr<Zone> _zone);
    void log(const std::string& _line, aLogSeverity _severity = lsDebug);
    bool initializeDeviceFromSpec(boost::shared_ptr<DSMeter> _dsMeter, boost::shared_ptr<Zone> _zone, DeviceSpec_t& _spec);
    bool initializeDeviceFromSpecQuick(boost::shared_ptr<DSMeter> _dsMeter, DeviceSpec_t& _spec);
    void scheduleDeviceReadout(const boost::shared_ptr<Device> _pDevice);
    void setMeterCapability(boost::shared_ptr<DSMeter> _dsMeter);
  private:
    Apartment& m_Apartment;
    StructureQueryBusInterface& m_Interface;
    ModelMaintenance& m_Maintenance;
  };

  class BinaryInputDeviceFilter : public IDeviceAction {
  private:
    std::vector<boost::shared_ptr<Device> > m_devs;
  public:
    BinaryInputDeviceFilter() {}
    virtual ~BinaryInputDeviceFilter() {}
    virtual bool perform(boost::shared_ptr<Device> _device) {
      if (_device->isPresent() && (_device->getBinaryInputCount() > 0)) {
        m_devs.push_back(_device);
      }
      return true;
    }
    std::vector<boost::shared_ptr<Device> > getDeviceList() {
      return m_devs;
    }
  };

  class BinaryInputScanner : public Task {
  public:
    BinaryInputScanner(Apartment* _papartment, const std::string& _busConnection);
    virtual ~BinaryInputScanner();

    virtual void run();
    virtual void setup(boost::shared_ptr<DSMeter> _dsMeter, boost::shared_ptr<Device> _device);

    uint16_t getDeviceSensorValue(const dsuid_t& _dsm,
                              dev_t _device,
                              uint8_t _sensorIndex) const;
    uint8_t getDeviceConfig(const dsuid_t& _dsm,
                              dev_t _device,
                              uint8_t _configClass,
                              uint8_t _configIndex) const;
  private:
    Apartment* m_pApartment;
    std::string m_busConnection;
    DsmApiHandle_t m_dsmApiHandle;
    boost::shared_ptr<DSMeter> m_dsm;
    boost::shared_ptr<Device> m_device;
  };

} // namespace dss

#endif // BUSSCANNER_H
