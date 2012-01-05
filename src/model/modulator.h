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
#include "src/ds485types.h"
#include "src/model/modeltypes.h"
#include "src/datetools.h"

namespace dss {
  class PropertyNode;
  typedef boost::shared_ptr<PropertyNode> PropertyNodePtr;
  class Apartment;

  /** Represents a DSMeter */
  class DSMeter : public DeviceContainer,
                  public PhysicalModelItem {
  private:
    dss_dsid_t m_DSID;
    DeviceVector m_ConnectedDevices;
    int m_EnergyLevelOrange;
    int m_EnergyLevelRed;
    int m_PowerConsumption;
    DateTime m_PowerConsumptionTimeStamp;
    int m_EnergyMeterValue;
    unsigned long m_LastReportedEnergyMeterValue;
    bool m_ReceivedMeterValue;
    DateTime m_EnergyMeterValueTimeStamp;
    int m_HardwareVersion;
    int m_armSoftwareVersion;
    int m_dspSoftwareVersion;
    int m_ApiVersion;
    std::string m_HardwareName;
    bool m_IsValid;
    bool m_IsInitialized;
    PropertyNodePtr m_pPropertyNode;
    Apartment* m_pApartment;
    unsigned int m_DatamodelHash;
    unsigned int m_DatamoderModificationCount;
  private:
    void publishToPropertyTree();
  public:
    /** Constructs a dsMeter with the given dsid. */
    DSMeter(const dss_dsid_t _dsid, Apartment* _pApartment);
    virtual ~DSMeter() {};
    virtual Set getDevices() const;

    /** Returns the DSID of the DSMeter */
    dss_dsid_t getDSID() const;

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
    void updateEnergyMeterValue(unsigned long _value);
    void initializeEnergyMeterValue(unsigned long _value);

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
    int getArmSoftwareVersion() const { return m_armSoftwareVersion; }
    void setArmSoftwareVersion(const int _value) { m_armSoftwareVersion = _value; }
    int getDspSoftwareVersion() const { return m_dspSoftwareVersion; }
    void setDspSoftwareVersion(const int _value) { m_dspSoftwareVersion = _value; }
    int getApiVersion() const { return m_ApiVersion; }
    void setApiVersion(const int _value) { m_ApiVersion = _value; }
    std::string getHardwareName() const { return m_HardwareName; }
    void setHardwareName(const std::string& _value) { m_HardwareName = _value; }

    unsigned int getDatamodelHash() const { return m_DatamodelHash; }
    void setDatamodelHash(const unsigned int hash) { m_DatamodelHash = hash; }
    unsigned int getDatamodelModificationCount() const { return m_DatamoderModificationCount; }
    void setDatamodelModificationcount(const unsigned int count) { m_DatamoderModificationCount = count; }

    /** Returns true if the dsMeter has been read-out completely. */
    bool isInitialized() const { return m_IsInitialized; }
    void setIsInitialized(const bool _value) { m_IsInitialized = _value; }

    bool isValid() const { return m_IsValid; }
    void setIsValid(const bool _value) { m_IsValid = _value; }

    PropertyNodePtr getPropertyNode() { return m_pPropertyNode; }
  }; // DSMeter


} // namespace dss


#endif // MODULATOR_H
