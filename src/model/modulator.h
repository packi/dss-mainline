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

#include <boost/enable_shared_from_this.hpp>
#include <string>
#include <bitset>

#include <boost/shared_ptr.hpp>

#include "base.h"
#include "devicecontainer.h"
#include "physicalmodelitem.h"
#include "src/ds485types.h"
#include "src/model/devicereference.h"
#include "src/datetools.h"
#include "src/storagetools.h"
#include "businterface.h"

#define DSM_POWER_STATES 8

namespace dss {
  class PropertyNode;
  typedef boost::shared_ptr<PropertyNode> PropertyNodePtr;
  class Apartment;
  class State;

  enum {
    DSM_APARTMENT_STATE_PRESENT = 0,
    DSM_APARTMENT_STATE_ABSENT  = 1,
    DSM_APARTMENT_STATE_UNKNOWN = 2
  };

  enum {
    DSM_STATE_IDLE          = 0,
    DSM_STATE_REGISTRATION  = 1,
    DSM_STATE_UNKNOWN       = 255
  };

  typedef struct {
    int m_index;              // power state index
    int m_activeValue;        // active threshold
    int m_inactiveValue;      // inactive threshold
    std::string m_name;       // state name
  } CircuitPowerState_t;

  bool busMemberIsAnyDSM(BusMemberDevice_t type);
  bool busMemberIsLogicDSM(BusMemberDevice_t type);
  bool busMemberIsHardwareDSM(BusMemberDevice_t type);
  bool busMemberTypeIsValid(BusMemberDevice_t type);

  /** Represents a DSMeter */
  class DSMeter : public boost::enable_shared_from_this<DSMeter>,
                  public DeviceContainer,
                  public PhysicalModelItem {
  private:
    dsuid_t m_DSID;

    /* dsm-api bus device type and capabilities */
    BusMemberDevice_t m_DeviceType;
    bool m_capHasDevices;
    bool m_capHasMetering;
    bool m_capHasTemperatureControl;

    std::vector<DeviceReference> m_ConnectedDevices;
    int m_PowerConsumption;
    DateTime m_PowerConsumptionTimeStamp;
    double m_EnergyMeterValue;
    double m_EnergyMeterValueWh;
    unsigned long long m_LastReportedEnergyMeterValue;
    bool m_ReceivedMeterValue;
    bool m_readStoredMeterValues;
    DateTime m_EnergyMeterValueTimeStamp;
    std::string m_HardwareVersion;
    int m_armSoftwareVersion;
    int m_dspSoftwareVersion;
    int m_ApiVersion;
    std::string m_HardwareName;
    bool m_IsValid;
    bool m_IsInitialized;
    bool m_HasPendingEvents;
    PropertyNodePtr m_pPropertyNode;
    Apartment* m_pApartment;
    unsigned int m_DatamodelHash;
    unsigned int m_DatamoderModificationCount;
    PersistentCounter m_BinaryInputEventCount;
    std::bitset<8> m_dSMPropertyFlags;
    bool m_IgnoreActionsFromNewDevices;
    uint8_t m_ApartmentState;
    std::string m_VdcConfigURL;
    std::string m_VdcModelUID;
    std::string m_VdcHardwareGuid;
    std::string m_VdcHardwareModelGuid;
    std::string m_VdcImplementationId;
    std::string m_VdcVendorGuid;
    std::string m_VdcOemGuid;
    std::string m_VdcOemModelGuid;
    std::string m_softwareVersion;
    int m_dSMState;
    bool m_synchronized;
    boost::shared_ptr<CircuitPowerState_t> m_powerStateConfigs[DSM_POWER_STATES];
  private:
    void publishToPropertyTree();
  public:
    /** Constructs a dsMeter with the given dsid. */
    DSMeter(const dsuid_t _dsid, Apartment* _pApartment);
    virtual ~DSMeter();
    virtual Set getDevices() const;

    /** Returns the DSID of the DSMeter */
    dsuid_t getDSID() const;

    /** returns a short id of the DSMeter */
    std::string getDisplayID() const;

    /** Adds a DeviceReference to the dsMeters devices list */
    void addDevice(const DeviceReference& _device);

    /** Removes the device identified by the reference. */
    void removeDevice(const DeviceReference& _device);

    /** Set and test capabilities */
    void setCapability_HasDevices(bool _flag) { m_capHasDevices = _flag; }
    bool getCapability_HasDevices() const { return m_capHasDevices; }
    void setCapability_HasMetering(bool _flag) { m_capHasMetering = _flag; }
    bool getCapability_HasMetering() const { return m_capHasMetering; }
    void setCapability_HasTemperatureControl(bool _flag) { m_capHasTemperatureControl = _flag; }
    bool getCapability_HasTemperatureControl() const { return m_capHasTemperatureControl; }

    void setBusMemberType(BusMemberDevice_t _devType) { m_DeviceType = _devType; }
    BusMemberDevice_t getBusMemberType() const { return m_DeviceType; }

    /** Returns the consumption in mW */
    unsigned long getPowerConsumption();
    /** Returns the meter value in Wh */
    unsigned long long getEnergyMeterValue();

    /** set the consumption in mW */
    void setPowerConsumption(unsigned long _value);
    /** set the meter value in Wh */
    void updateEnergyMeterValue(unsigned long _value);
    void initializeEnergyMeterValue(unsigned long long _value);

    /** Returns the last consumption in mW returned from dS485 Bus, but never request it*/
    unsigned long getCachedPowerConsumption();

    /** Returns timestamp of the last consumption measurement */
    const DateTime& getCachedPowerConsumptionTimeStamp() const;

    /** Returns the last meter value in Wh returned from dS485 Bus, but never request it*/
    double getCachedEnergyMeterValue();

    /** Returns timestamp of the last energy measurement */
    const DateTime& getCachedEnergyMeterTimeStamp() const;

    void setSoftwareVersion(const std::string _version) { m_softwareVersion = _version; }
    std::string getSoftwareVersion() const;
    std::string getHardwareVersion() const { return m_HardwareVersion; }
    void setHardwareVersion(const int _value) { m_HardwareVersion = intToString((_value >> 24) & 0xFF) + "." + intToString((_value >> 16) & 0xFF) + "." + intToString((_value >> 8) & 0xFF) + "." + intToString(_value & 0xFF); }
    void setHardwareVersion(const std::string _value) { m_HardwareVersion = _value; }
    int getArmSoftwareVersion() const { return m_armSoftwareVersion; }
    void setArmSoftwareVersion(const int _value) { m_armSoftwareVersion = _value; }
    std::string getDspSoftwareVersionStr() const { return intToString((m_dspSoftwareVersion >> 24) & 0xFF) + "." + intToString((m_dspSoftwareVersion >> 16) & 0xFF) + "." + intToString((m_dspSoftwareVersion >> 8) & 0xFF) + "." + intToString(m_dspSoftwareVersion & 0xFF); }
    int getDspSoftwareVersion() const { return m_dspSoftwareVersion; }
    void setDspSoftwareVersion(const int _value) { m_dspSoftwareVersion = _value; }
    int getApiVersion() const { return m_ApiVersion; }
    void setApiVersion(const int _value) { m_ApiVersion = _value; }
    std::string getHardwareName() const;
    void setHardwareName(const std::string& _value) { m_HardwareName = _value; }

    unsigned int getDatamodelHash() const { return m_DatamodelHash; }
    void setDatamodelHash(const unsigned int hash) { m_DatamodelHash = hash; }
    unsigned int getDatamodelModificationCount() const { return m_DatamoderModificationCount; }
    void setDatamodelModificationcount(const unsigned int count) { m_DatamoderModificationCount = count; }
    unsigned int getBinaryInputEventCount() const { return m_BinaryInputEventCount.getValue(); }
    void setBinaryInputEventCount(const unsigned int _value) { m_BinaryInputEventCount.setValue(_value); }
    void incrementBinaryInputEventCount() { m_BinaryInputEventCount.increment(); }
    void setPropertyFlags(std::bitset<8> _flags);
    std::bitset<8> getPropertyFlags() const { return m_dSMPropertyFlags; }
    /** Returns true if the dsMeter has been read-out completely. */
    bool isInitialized() const { return m_IsInitialized; }
    void setIsInitialized(const bool _value) { m_IsInitialized = _value; }

    bool isValid() const { return m_IsValid; }
    void setIsValid(const bool _value) { m_IsValid = _value; }

    bool hasPendingEvents() const { return m_HasPendingEvents; }
    void setHasPendingEvents(const bool _value) { m_HasPendingEvents = _value; }

    PropertyNodePtr getPropertyNode() { return m_pPropertyNode; }

    std::string getEnergyMeterValueWhAsString() const { return intToString(m_EnergyMeterValueWh); }
    std::string getEnergyMeterValueAsString() const { return intToString(m_EnergyMeterValue); }
    void setApartmentState(uint8_t _state) { m_ApartmentState = _state; }
    uint8_t getApartmentState() const { return m_ApartmentState; }
    void setVdcConfigURL(const std::string& _value) { m_VdcConfigURL = _value; }
    const std::string& getVdcConfigURL() const { return m_VdcConfigURL; }
    void setVdcModelUID(const std::string& _value) { m_VdcModelUID = _value; }
    const std::string& getVdcModelUID() const { return m_VdcModelUID; }
    void setVdcHardwareGuid(const std::string& _value ) { m_VdcHardwareGuid = _value; }
    const std::string& getVdcHardwareGuid() const { return m_VdcHardwareGuid; }
    void setVdcHardwareModelGuid(const std::string& _value ) { m_VdcHardwareModelGuid = _value; }
    const std::string& getVdcHardwareModelGuid() const { return m_VdcHardwareModelGuid; }
    void setVdcImplementationId(const std::string& _value ) { m_VdcImplementationId = _value; }
    const std::string& getVdcImplementationId() const { return m_VdcImplementationId; }
    void setVdcVendorGuid(const std::string& _value ) { m_VdcVendorGuid = _value; }
    const std::string& getVdcVendorGuid() const { return m_VdcVendorGuid; }
    void setVdcOemGuid(const std::string& _value ) { m_VdcOemGuid = _value; }
    const std::string& getVdcOemGuid() const { return m_VdcOemGuid; }
    void setVdcOemModelGuid(const std::string& _value ) { m_VdcOemModelGuid = _value; }
    const std::string& getVdcOemModelGuid() const { return m_VdcOemModelGuid; }

    int getState() const { return m_dSMState; }
    void setState(uint8_t _state) { m_dSMState = _state; }
    
    void setSynchronized() { m_synchronized = true; }
    bool isSynchonized() const { return m_synchronized; }

    boost::shared_ptr<State> getPowerState(uint8_t _inputIndex) const;
    bool setPowerState(const int _index, const int _setThreshold, const int _resetThreshold);
    void setPowerStates(const std::vector<CircuitPowerStateSpec_t>& _powerStateConfigs);
  }; // DSMeter


} // namespace dss


#endif // MODULATOR_H
