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


#ifndef MODULATOR_H
#define MODULATOR_H

#include <string>

#include <boost/shared_ptr.hpp>

#include "devicecontainer.h"
#include "physicalmodelitem.h"
#include "core/ds485types.h"
#include "core/model/modeltypes.h"
#include "core/datetools.h"

namespace dss {
  class PropertyNode;
  typedef boost::shared_ptr<PropertyNode> PropertyNodePtr;
  class Apartment;

  /** Represents a DSMeter */
  class DSMeter : public DeviceContainer,
                  public PhysicalModelItem {
  private:
    dsid_t m_DSID;
    int m_BusID;
    DeviceVector m_ConnectedDevices;
    int m_EnergyLevelOrange;
    int m_EnergyLevelRed;
    int m_PowerConsumption;
    DateTime m_PowerConsumptionTimeStamp;
    int m_EnergyMeterValue;
    DateTime m_EnergyMeterValueTimeStamp;
    int m_HardwareVersion;
    int m_SoftwareVersion;
    std::string m_HardwareName;
    int m_DeviceType;
    bool m_IsValid;
    PropertyNodePtr m_pPropertyNode;
    Apartment* m_pApartment;
  private:
    void publishToPropertyTree();
  public:
    /** Constructs a dsMeter with the given dsid. */
    DSMeter(const dsid_t _dsid, Apartment* _pApartment);
    virtual ~DSMeter() {};
    virtual Set getDevices() const;

    /** Returns the DSID of the DSMeter */
    dsid_t getDSID() const;
    /** Returns the bus id of the DSMeter */
    int getBusID() const;
    /** Sets the bus id of the DSMeter */
    void setBusID(const int _busID);

    /** Adds a DeviceReference to the dsMeters devices list */
    void addDevice(const DeviceReference& _device);

    /** Removes the device identified by the reference. */
    void removeDevice(const DeviceReference& _device);

    /** Returns the consumption in mW */
    unsigned long getPowerConsumption();
    /** Returns the meter value in Wh */
    unsigned long getEnergyMeterValue();


    /** set the consumption in mW */
    void setPowerConsumption(unsigned long _value);
    /** set the meter value in Wh */
    void setEnergyMeterValue(unsigned long _value);

    /** Returns the last consumption in mW returned from dS485 Bus, but never request it*/
    unsigned long getCachedPowerConsumption();

    /** Returns timestamp of the last consumption measurement */
    const DateTime& getCachedPowerConsumptionTimeStamp() const;

    /** Returns the last meter value in Wh returned from dS485 Bus, but never request it*/
    unsigned long getCachedEnergyMeterValue();

    /** Returns timestamp of the last energy measurement */
    const DateTime& getCachedEnergyMeterTimeStamp() const;

    /** Returns the orange energy level */
    int getEnergyLevelOrange() const { return m_EnergyLevelOrange; }
    /** Returns the red energy level */
    int getEnergyLevelRed() const { return m_EnergyLevelRed; }
    /** Sets the orange energy level.
     * @note This has no effect on the dsMeter as of now. */
    void setEnergyLevelRed(const int _value) { m_EnergyLevelRed = _value; }
    /** Sets the red energy level.
     * @note This has no effect on the dsMeter as of now. */
    void setEnergyLevelOrange(const int _value) { m_EnergyLevelOrange = _value; }

    int getHardwareVersion() const { return m_HardwareVersion; }
    void setHardwareVersion(const int _value) { m_HardwareVersion = _value; }
    int getSoftwareVersion() const { return m_SoftwareVersion; }
    void setSoftwareVersion(const int _value) { m_SoftwareVersion = _value; }
    std::string getHardwareName() const { return m_HardwareName; }
    void setHardwareName(const std::string& _value) { m_HardwareName = _value; }
    int getDeviceType() { return m_DeviceType; }
    void setDeviceType(const int _value) { m_DeviceType = _value; }

    /** Returns true if the dsMeter has been read-out completely. */
    bool isValid() const { return m_IsValid; }
    void setIsValid(const bool _value) { m_IsValid = _value; }

    PropertyNodePtr getPropertyNode() { return m_pPropertyNode; }
  }; // DSMeter


} // namespace dss


#endif // MODULATOR_H
