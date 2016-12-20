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

#ifndef ZONE_H
#define ZONE_H

#include <vector>

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

#include "devicecontainer.h"
#include "nonaddressablemodelitem.h"
#include "physicalmodelitem.h"
#include "storagetools.h"
#include "businterface.h"

namespace dss {
  class Apartment;
  class DSMeter;
  class Group;
  class Set;
  class DeviceReference;
  class PropertyNode;
  typedef boost::shared_ptr<PropertyNode> PropertyNodePtr;
  class DateTime;

  typedef struct ZoneHeatingProperties {
    ZoneHeatingProperties() :
      m_HeatingControlMode(0),
      m_Kp(0),
      m_Ts(0),
      m_Ti(0),
      m_Kd(0),
      m_Imin(0),
      m_Imax(0),
      m_Ymin(0),
      m_Ymax(0),
      m_AntiWindUp(0),
      m_KeepFloorWarm(0),
      m_HeatingControlState(0),
      m_HeatingMasterZone(0),
      m_CtrlOffset(0),
      m_EmergencyValue(175),
      m_ManualValue(0),
      m_HeatingControlDSUID(DSUID_NULL)
      {
      }
    void reset() {
      m_HeatingControlMode = 0;
      m_Kp = 0;
      m_Ts = 0;
      m_Ti = 0;
      m_Kd = 0;
      m_Imin = 0;
      m_Imax = 0;
      m_Ymin =0;
      m_Ymax = 0;
      m_AntiWindUp =0;
      m_KeepFloorWarm =0;
      m_HeatingControlState = 0;
      m_HeatingMasterZone = 0;
      m_CtrlOffset = 0;
      m_EmergencyValue = 175;
      m_ManualValue = 0;
      m_HeatingControlDSUID = DSUID_NULL;
    }
    bool isEqual(const ZoneHeatingConfigSpec_t& _config) {
      return ((_config.ControllerMode == m_HeatingControlMode) &&
              (_config.Kp == m_Kp) &&
              (_config.Ts == m_Ts) &&
              (_config.Ti == m_Ti) &&
              (_config.Kd == m_Kd) &&
              (_config.Imin == m_Imin) &&
              (_config.Imax == m_Imax) &&
              (_config.Ymin == m_Ymin) &&
              (_config.Ymax == m_Ymax) &&
              (_config.AntiWindUp == m_AntiWindUp) &&
              (_config.KeepFloorWarm == m_KeepFloorWarm) &&
              (_config.SourceZoneId == m_HeatingMasterZone) &&
              (_config.Offset == m_CtrlOffset) &&
              (_config.EmergencyValue == m_EmergencyValue) &&
              (_config.ManualValue == m_ManualValue));
    }
    int m_HeatingControlMode;      // Control mode: 0=off; 1=pid-control; 2=zone-follower; 3=fixed-value; 4=manual
    uint16_t m_Kp;
    uint8_t  m_Ts;
    uint16_t m_Ti;
    uint16_t m_Kd;
    int16_t  m_Imin;
    int16_t  m_Imax;
    uint8_t  m_Ymin;
    uint8_t  m_Ymax;
    uint8_t  m_AntiWindUp;
    uint8_t  m_KeepFloorWarm;
    int m_HeatingControlState;     // Control state: 0=internal; 1=external; 2=exbackup; 3=emergency
    int m_HeatingMasterZone;       // only used for mode 2
    int m_CtrlOffset;              // only used for mode 2
    uint8_t  m_EmergencyValue;
    int m_ManualValue;             // only used for mode 4
    dsuid_t m_HeatingControlDSUID; // DSUID of the meter or device or service running a controller for this zone
  } ZoneHeatingProperties_t;

  typedef struct ZoneHeatingStatus {
    ZoneHeatingStatus() :
      m_NominalValue(0),
      m_NominalValueTS(DateTime::NullDate),
      m_ControlValue(0),
      m_ControlValueTS(DateTime::NullDate)
      {}
    double m_NominalValue;         // only used for mode 1
    DateTime m_NominalValueTS;
    double m_ControlValue;
    DateTime m_ControlValueTS;
  } ZoneHeatingStatus_t;

  typedef struct ZoneSensorStatus {
    ZoneSensorStatus() :
      m_TemperatureValue(0),
      m_TemperatureValueTS(DateTime::NullDate),
      m_HumidityValue(0),
      m_HumidityValueTS(DateTime::NullDate),
      m_BrightnessValue(0),
      m_BrightnessValueTS(DateTime::NullDate),
      m_CO2ConcentrationValue(0),
      m_CO2ConcentrationValueTS(DateTime::NullDate)
      {}
    double m_TemperatureValue;
    DateTime m_TemperatureValueTS;
    double m_HumidityValue;
    DateTime m_HumidityValueTS;
    double m_BrightnessValue;
    DateTime m_BrightnessValueTS;
    double m_CO2ConcentrationValue;
    DateTime m_CO2ConcentrationValueTS;
  } ZoneSensorStatus_t;

  typedef struct {
    dsuid_t m_DSUID;
    SensorType m_sensorType;
    int m_sensorIndex;
  } MainZoneSensor_t;
    /** Represents a Zone.
   * A Zone houses multiple devices. It can span over multiple dsMeters.
   */
  class Zone : public DeviceContainer,
               public NonAddressableModelItem,
               public PhysicalModelItem,
               public boost::noncopyable {
  private:
    int m_ZoneID;
    std::vector<DeviceReference> m_Devices;
    std::vector<boost::shared_ptr<const DSMeter> > m_DSMeters;
    std::vector<boost::shared_ptr<Group> > m_Groups;
    std::vector<MainZoneSensor_t> m_MainSensors;
    ZoneHeatingProperties_t m_HeatingProperties;
    ZoneHeatingStatus_t m_HeatingStatus;
    PersistentValue<int> m_HeatingOperationMode;
    ZoneSensorStatus_t m_SensorStatus;
    Apartment* m_pApartment;
    PropertyNodePtr m_pPropertyNode;
    bool m_HeatingPropValid;

  public:
    Zone(const int _id, Apartment* _pApartment);
    virtual ~Zone();
    virtual Set getDevices() const;

    Apartment& getApartment() { return *m_pApartment; }

    /** Adds the Zone to a dsMeter. */
    void addToDSMeter(boost::shared_ptr<DSMeter> _dsMeter);
    /** Removes the Zone from a dsMeter. */
    void removeFromDSMeter(boost::shared_ptr<DSMeter> _dsMeter);

    /** Adds a device to the zone.
     * This will permanently add the device to the zone.
     */
    void addDevice(DeviceReference& _device);

    /** Removes a device from the zone.
     * This will permanently remove the device from the zone.
     */
    void removeDevice(const DeviceReference& _device);

    /** Returns the group with the name \a _name */
    boost::shared_ptr<Group> getGroup(const std::string& _name) const;
    /** Returns the group with the id \a _id */
    boost::shared_ptr<Group> getGroup(const int _id) const;

    /** Adds a group to the zone */
    void addGroup(boost::shared_ptr<Group> _group);
    /** Removes a group from the zone */
    void removeGroup(boost::shared_ptr<Group> _group);

    /** Returns the zones id */
    int getID() const;
    /** Sets the zones id */
    void setZoneID(const int _value);

    bool registeredOnDSMeter(boost::shared_ptr<const DSMeter> _dsMeter) const;
    bool isRegisteredOnAnyMeter() const;
    std::vector<boost::shared_ptr<const DSMeter> > getDSMeters() { return m_DSMeters; }

    virtual void nextScene(const callOrigin_t _origin, const SceneAccessCategory _category);
    virtual void previousScene(const callOrigin_t _origin, const SceneAccessCategory _category);

    void publishToPropertyTree();
    void removeFromPropertyTree();
    PropertyNodePtr getPropertyNode() {  return m_pPropertyNode; }

    virtual unsigned long getPowerConsumption();

    /** Returns a vector of groups present on the zone. */
    std::vector<boost::shared_ptr<Group> > getGroups() { return m_Groups; }

    /** Returns the heating  and sensor properties */
    ZoneHeatingProperties_t getHeatingProperties() const;
    ZoneHeatingStatus_t getHeatingStatus() const;
    ZoneSensorStatus_t getSensorStatus() const;
    bool isHeatingEnabled() const;

    /** Set heating properties and runtime values */
    void setHeatingControlMode(const ZoneHeatingConfigSpec_t _spec, dsuid_t ctrlDevice);
    ZoneHeatingConfigSpec_t getHeatingControlMode();
    void clearHeatingControlMode();
    void setHeatingControlState(int _ctrlState);
    void setHeatingOperationMode(int _operationMode);
    int getHeatingOperationMode() const;
    void setTemperature(double _value, DateTime& _ts);
    void setNominalValue(double _value, DateTime& _ts);
    void setControlValue(double _value, DateTime& _ts);
    void setHumidityValue(double _value, DateTime& _ts);
    void setBrightnessValue(double _value, DateTime& _ts);
    void setCO2ConcentrationValue(double _value, DateTime& _ts);

    void setSensor(const Device &_device, SensorType _sensorType);
    void setSensor(const MainZoneSensor_t &_mainZoneSensor);
    void resetSensor(SensorType _sensorType);
    bool isSensorAssigned(SensorType _sensorType) const;
    const std::vector<MainZoneSensor_t>& getAssignedSensors() const { return m_MainSensors; }

    std::vector<SensorType> getUnassignedSensorTypes() const;
    std::vector<SensorType> getAssignedSensorTypes(const Device &_device) const;

    boost::shared_ptr<Device> getAssignedSensorDevice(SensorType _sensorType) const;
    bool isZoneSensor(const Device &_device, SensorType _sensorType) const;

    bool isDeviceZoneMember(const DeviceReference& _device) const;
    void removeInvalidZoneSensors();

    /** Set heating properties */
    void setHeatingProperties(ZoneHeatingProperties_t& config);
    bool isHeatingPropertiesValid() const;

  protected:
    virtual std::vector<boost::shared_ptr<AddressableModelItem> > splitIntoAddressableItems();
    bool isAllowedSensorType(SensorType _sensorType);
    void dirty();
  }; // Zone


} // namespace dss

#endif // ZONE_H
