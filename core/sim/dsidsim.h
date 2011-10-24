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

#ifndef DSIDSIM_H_
#define DSIDSIM_H_

#include "dssim.h"

#include "core/base.h"

namespace dss {

  class DSIDSim : public DSIDInterface {
  private:
    bool m_On;
    bool m_Enabled;
    bool m_Dimming;
    time_t m_DimmStartTime;
    bool m_DimmingUp;
    DSMeterSim* m_DSMeter;
    std::vector<uint8_t> m_ValuesForScene;
    uint8_t m_CurrentValue;
    std::map<int, std::map<int, int> > m_DeviceConfig;
    int m_DimTimeMS;
    int m_SimpleConsumption;
    Properties m_ConfigParameter;
  public:
    DSIDSim(const DSMeterSim& _simulator, const dss_dsid_t _dsid, const devid_t _shortAddress);
    virtual ~DSIDSim() {}

    virtual void callScene(const int _sceneNr);
    virtual void saveScene(const int _sceneNr);
    virtual void undoScene(const int _sceneNr);
    virtual void undoSceneLast();

    virtual void enable();
    virtual void disable();

    virtual uint32_t getPowerConsumption();
    virtual uint32_t getEnergyMeterValue();

    virtual void setDeviceConfig(const uint8_t _configClass,
                                 const uint8_t _configIndex,
                                 const uint8_t _value);

    virtual uint8_t getDeviceConfig(const uint8_t _configClass,
                                    const uint8_t _configIndex);

    virtual uint16_t getDeviceConfigWord(const uint8_t _configClass,
                                       const uint8_t _configIndex);

    virtual void setDeviceProgMode(const uint8_t _modeId);

    virtual uint32_t getDeviceSensorValue(const uint8_t _sensorIndex);
    virtual uint8_t getDeviceSensorType(const uint8_t _sensorIndex);
    virtual void sensorPush(const uint8_t _sensorType, const uint16_t _sensorValue);

    virtual void setValue(const uint8_t _value);

    virtual std::pair<uint8_t, uint16_t> getTransmissionQuality();

    virtual uint16_t getFunctionID();
    virtual uint16_t getProductID();
    virtual uint16_t getProductRevision();

    void setSimpleConsumption(const int _value) { m_SimpleConsumption = _value; }
    virtual void setConfigParameter(const std::string& _name, const std::string& _value);
    virtual std::string getConfigParameter(const std::string& _name) const;
  }; // DSIDSim

}

#endif /* DSIDSIM_H_ */
