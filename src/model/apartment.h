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

#ifndef APARTMENT_H
#define APARTMENT_H

#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <regex.h>
#include <set>

#include "devicecontainer.h"
#include "src/subsystem.h"
#include "src/model/state.h"
#include "src/datetools.h"
#include "src/model/device.h"

namespace dss {
  class PropertyNode;
  typedef boost::shared_ptr<PropertyNode> PropertyNodePtr;
  class Zone;
  class DSMeter;
  class Group;
  class Cluster;
  class Event;
  class ModelMaintenance;
  class PropertySystem;
  class BusInterface;
  class Metering;
  class DeviceBusInterface;
  class ActionRequestInterface;

  typedef struct ApartmentSensorStatus {
    ApartmentSensorStatus() :
      m_TemperatureValue(0),
      m_TemperatureTS(0),
      m_HumidityValue(0),
      m_HumidityTS(0),
      m_BrightnessValue(0),
      m_BrightnessTS(0),
      m_WindSpeedValue(0),
      m_WindSpeedTS(0),
      m_WindDirectionValue(0),
      m_WindDirectionTS(0),
      m_GustSpeedValue(0),
      m_GustSpeedTS(0),
      m_GustDirectionValue(0),
      m_GustDirectionTS(0),
      m_PrecipitationValue(0),
      m_PrecipitationTS(0),
      m_AirPressureValue(0),
      m_AirPressureTS(0),
      m_WeatherTS(0)
      {}
    double m_TemperatureValue;
    DateTime m_TemperatureTS;
    double m_HumidityValue;
    DateTime m_HumidityTS;
    double m_BrightnessValue;
    DateTime m_BrightnessTS;
    double m_WindSpeedValue;
    DateTime m_WindSpeedTS;
    double m_WindDirectionValue;
    DateTime m_WindDirectionTS;
    double m_GustSpeedValue;
    DateTime m_GustSpeedTS;
    double m_GustDirectionValue;
    DateTime m_GustDirectionTS;
    double m_PrecipitationValue;
    DateTime m_PrecipitationTS;
    double m_AirPressureValue;
    DateTime m_AirPressureTS;

    std::string m_WeatherIconId;
    std::string m_WeatherConditionId;
    std::string m_WeatherServiceId;
    DateTime m_WeatherTS;
  } ApartmentSensorStatus_t;


typedef struct {
    DeviceClasses_t deviceClass;
    std::set<int> lockedScenes;
} ClassLock_t;

typedef struct {
  int zoneID;
  std::vector<ClassLock_t> deviceClassLocks;
} ZoneLock_t;

typedef struct {
  dsuid_t dsuid;
  std::vector<int> lockedScenes;
} DeviceLock_t;
  
  /** Represents an Apartment
    * This is the root of the datamodel of the dss. The Apartment is responsible for delivering
    * and loading all subitems.
    */
  class Apartment : public boost::noncopyable,
                    public DeviceContainer
  {
  private:
    DSS* m_dss;
    std::vector<boost::shared_ptr<Zone> > m_Zones;
    std::vector<boost::shared_ptr<DSMeter> > m_DSMeters;
    std::vector<boost::shared_ptr<Device> > m_Devices;
    std::vector<boost::shared_ptr<State> > m_States;
    ApartmentSensorStatus_t m_SensorStatus;

    BusInterface* m_pBusInterface;
    PropertyNodePtr m_pPropertyNode;
    ModelMaintenance* m_pModelMaintenance;
    PropertySystem* m_pPropertySystem;
    Metering* m_pMetering;
    mutable boost::recursive_mutex m_mutex;
  private:
    void addDefaultGroupsToZone(boost::shared_ptr<Zone> _zone);
  public:
    Apartment(DSS* _pDSS);
    virtual ~Apartment();

    boost::recursive_mutex& getMutex() const { return m_mutex; }
    DSS& getDss() { return *m_dss; }

    /** Returns a set containing all devices of the set */
    virtual Set getDevices() const;

    /** Returns a reference to the device with the DSID \a _dsid */
    boost::shared_ptr<Device> getDeviceByDSID(const dsuid_t _dsid) const;
    /** Returns a reference to the device with the name \a _name*/
    boost::shared_ptr<Device> getDeviceByName(const std::string& _name);
    std::vector<boost::shared_ptr<Device> > getDevicesVector() { return m_Devices; }

    /** Returns the Zone by name */
    boost::shared_ptr<Zone> getZone(const std::string& _zoneName);
    boost::weak_ptr<Zone> tryGetZone(const int id);
    /** Returns the Zone by its id */
    boost::shared_ptr<Zone> getZone(const int _id);
    /** Returns a vector of all zones */
    std::vector<boost::shared_ptr<Zone> > getZones();

    /** Returns a DSMeter by name */
    boost::shared_ptr<DSMeter> getDSMeter(const std::string& _modName);
    /** Returns a DSMeter by DSID  */
    boost::shared_ptr<DSMeter> getDSMeterByDSID(const dsuid_t _dsid);
    /** Returns a vector of all dsMeters */
    std::vector<boost::shared_ptr<DSMeter> > getDSMeters();

    /** Returns a Group by name */
    boost::shared_ptr<Group> getGroup(const std::string& _name);
    /** Returns a Group by id */
    boost::shared_ptr<Group> getGroup(const int _id);

    /** Returns a Cluster by id */
    boost::shared_ptr<Cluster> getCluster(const int _id);
    /** Returns a vector of Clusters. */
    std::vector<boost::shared_ptr<Cluster> > getClusters();
    /** Return an Empty Cluster, if available. */
    boost::shared_ptr<Cluster> getEmptyCluster();

    /** Returns a vector of Global Application Groups */
    std::vector<boost::shared_ptr<Group> > getGlobalApps();

    /// @return nullptr on failure
    boost::shared_ptr<State> tryGetState(const eStateType _type, const std::string& _stateName) const;
    /// @throw ItemNotFoundException on failure
    boost::shared_ptr<State> getState(const eStateType _type,
                                      const std::string& _stateName) const;
    boost::shared_ptr<State> getNonScriptState(const std::string& _stateName) const;
    /** Returns the state by type, identifier and name */
    boost::shared_ptr<State> getState(const eStateType _type,
                                      const std::string& _identifier,
                                      const std::string& _stateName) const;

    void updateSensorStates(const dsuid_t &dsuid, SensorType sensorType, double value, callOrigin_t origin);
    void updateSensorStates(int zoneId, int groupId, SensorType sensorType, double value, callOrigin_t origin);

    /** Allocates a device and returns a reference to it. */
    boost::shared_ptr<Device> allocateDevice(const dsuid_t _dsid);
    /** Allocates a zone and returns a reference to it. Should a zone with
      * the given _zoneID already exist, a reference to the existing zone will
      * be returned.
      * NOTE: Outside code should never call this function
      */
    boost::shared_ptr<Zone> allocateZone(int _zoneID);
    boost::shared_ptr<DSMeter> allocateDSMeter(const dsuid_t _dsid);
    boost::shared_ptr<State> allocateState(eStateType _type, const std::string& _name, const std::string& _scriptId);
    void allocateState(boost::shared_ptr<State> _state);

    void removeZone(int _zoneID);
    void removeDevice(dsuid_t _device);
    void removeDSMeter(dsuid_t _dsMeter);
    void removeInactiveMeters();
    void removeState(boost::shared_ptr<State> _state);

    ApartmentSensorStatus_t getSensorStatus() const;
    void setTemperature(double _value, DateTime& _ts);
    void setHumidityValue(double _value, DateTime& _ts);
    void setBrightnessValue(double _value, DateTime& _ts);
    void setWindSpeed(double _value, DateTime& _ts);
    void setWindDirection(double _value, DateTime& _ts);
    void setGustSpeed(double _value, DateTime& _ts);
    void setGustDirection(double _value, DateTime& _ts);
    void setPrecipitation(double _value, DateTime& _ts);
    void setAirPressure(double _value, DateTime& _ts);
    void setWeatherInformation(std::string& _iconId, std::string& _conditionId, std::string _serviceId, DateTime& _ts);
    std::pair<std::vector<DeviceLock_t>, std::vector<ZoneLock_t> > getClusterLocks();

    bool setDevicesFirstSeen(const DateTime& dateTime);

  public:
    void setBusInterface(BusInterface* _value) { m_pBusInterface = _value; }
    BusInterface* getBusInterface() { return m_pBusInterface; }
    ActionRequestInterface* getActionRequestInterface();
    DeviceBusInterface* getDeviceBusInterface();
    /** Returns the root-node for the apartment tree */
    PropertyNodePtr getPropertyNode() { return m_pPropertyNode; }
    void setModelMaintenance(ModelMaintenance* _value) { m_pModelMaintenance = _value; }
    ModelMaintenance* getModelMaintenance() { return m_pModelMaintenance; }
    void setMetering(Metering* _value) { m_pMetering = _value; }
    void setPropertySystem(PropertySystem* _value);
    PropertySystem* getPropertySystem() { return m_pPropertySystem; }
  }; // Apartment

  /** Exception that will be thrown if a given item could not be found */
  class ItemNotFoundException : public DSSException {
  public:
    ItemNotFoundException(const std::string& _name) : DSSException(std::string("Could not find item ") + _name) {};
    ~ItemNotFoundException() throw() {}
  };

  /** Exception that will be thrown if a given item is duplicate in the data model */
  class ItemDuplicateException : public DSSException {
  public:
    ItemDuplicateException(const std::string& _name) : DSSException(std::string("Duplicate item ") + _name) {};
    ~ItemDuplicateException() throw() {}
  };

} // namespace dss

#endif // APARTMENT_H
