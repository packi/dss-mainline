/*
 *  model.cpp
 *  dSS
 *
 *  Copyright:
 *  (c) 2008 by
 *  futureLAB AG
 *  Schwalmenackerstrasse 4
 *  CH-8400 Winterthur / Schweiz
 *  Alle Rechte vorbehalten.
 *  Jede Art der Vervielfaeltigung, Verbreitung,
 *  Auswertung oder Veraenderung - auch auszugsweise -
 *  ist ohne vorgaengige schriftliche Genehmigung durch
 *  die futureLAB AG untersagt.
 *
 * Last change $Date: 2008-04-07 14:16:35 +0200 (Mon, 07 Apr 2008) $
 * by $Author: pstaehlin $
 */

#include "model.h"
#include "dss.h"

namespace dss {
  
  //============================================= Device
  
  Device::Device(devid_t _id)
  : m_ID(_id)
  {
  }
  
  void Device::TurnOn() {
    //DSS::GetInstance()->GetDS485Proxy().TurnOn( *this );
  } // TurnOn
  
  void Device::TurnOff() {
  } // TurnOff
  
  void Device::IncreaseValue(const int _parameterNr) {
  } // IncreaseValue
  
  void Device::DecreaseValue(const int _parameterNr) {
  } // DecreaseValue
  
  void Device::Enable() {
  } // Enable
  
  void Device::Disable() {
  } // Disable
  
  void Device::StartDim(const bool _directionUp, const int _parameterNr) {
  } // StartDim
  
  void Device::EndDim(const int _parameterNr) {
  } // EndDim
  
  void Device::SetValue(const double _value, const int _parameterNr) {
  } // SetValue
  
  double Device::GetValue(const int _parameterNr) {
    return -1;
  } // GetValue
  
  string Device::GetName() const {
    return m_Name;
  } // GetName
  
  void Device::SetName(const string& _name) {
    m_Name = _name;
  } // SetName
  
  bool Device::operator==(const Device& _other) const {
    return _other.m_ID == m_ID;
  } // operator==
  
  devid_t Device::GetID() const {
    return m_ID;
  } // GetID
  //================================================== Set
  
  Set::Set() {
  } // ctor
  
  Set::Set(Device _device) {
  } // ctor(Device)
  
  Set::Set(DeviceVector _devices) {
  } // ctor(DeviceVector)
  
  void Set::TurnOn() {
  } // TurnOn
  
  void Set::TurnOff() {
  } // TurnOff
  
  void Set::IncreaseValue(const int _parameterNr) {
  } // IncreaseValue
  
  void Set::DecreaseValue(const int _parameterNr) {
  } // DecreaseValue
  
  void Set::Enable() {
  } // Enable
  
  void Set::Disable() {
  } // Disable
  
  void Set::StartDim(bool _directionUp, const int _parameterNr) {
  } // StartDim
  
  void Set::EndDim(const int _parameterNr) {
  } // EndDim
  
  void Set::SetValue(const double _value, int _parameterNr) {
  } // SetValue
  //HashMapDeviceDouble Set::GetValue(const int _parameterNr);
  
  void Set::Perform(const IDeviceAction& _deviceAction) {
  } // Perform
  
  Set Set::GetSubset(const IDeviceSelector& _selector) {
    Set result;
    for(DeviceIterator iDevice = m_ContainedDevices.begin(); iDevice != m_ContainedDevices.end(); ++iDevice) {
      if(_selector.SelectDevice(*iDevice)) {
        result.AddDevice(*iDevice);
      }
    }
    return result;
  } // GetSubset
  
  Set Set::GetByGroup(int _groupNr) {
  } // GetByGroup
  
  class ByNameSelector : public IDeviceSelector {
  private:
    const string m_Name;
  public:
    ByNameSelector(const string& _name) : m_Name(_name) {};
    
    virtual bool SelectDevice(const Device& _device) const {
      return _device.GetName() == m_Name;
    }
  };
  
  Device Set::GetByName(const string& _name) {
    Set resultSet = GetSubset(ByNameSelector(_name));
    if(resultSet.Length() == 0) {
      throw ItemNotFoundException(_name);
    }
    return resultSet.m_ContainedDevices.front();
  } // GetByName
  
  
  class ByIDSelector : public IDeviceSelector {
  private:
    const int m_ID;
  public:
    ByIDSelector(const int _id) : m_ID(_id) {}
    
    virtual bool SelectDevice(const Device& _device) const {
      return _device.GetID() == m_ID;
    }
  };
  
  Device Set::GetByID(const int _id) {
    Set resultSet = GetSubset(ByIDSelector(_id));
    if(resultSet.Length() == 0) {
      throw ItemNotFoundException(string("with id ") + IntToString(_id));
    }
    return resultSet.m_ContainedDevices.front();
  } // GetByID
  
  int Set::Length() const {
    return m_ContainedDevices.size();
  } // Length
  
  Set Set::Combine(Set& _other) const {
    Set resultSet(_other);
    for(DeviceVector::const_iterator iDevice = m_ContainedDevices.begin(); iDevice != m_ContainedDevices.end(); ++iDevice) {
      resultSet.AddDevice(*iDevice);
    }
    return resultSet;
  } // Combine
  
  Set Set::Remove(Set& _other) const {
    Set resultSet(*this);
    for(DeviceIterator iDevice = _other.m_ContainedDevices.begin(); iDevice != _other.m_ContainedDevices.end(); ++iDevice) {
      resultSet.RemoveDevice(*iDevice);
    }    
  } // Remove
  
  void Set::AddDevice(const Device& _device) {
    m_ContainedDevices.push_back(_device);
  } // AddDevice
  
  void Set::RemoveDevice(const Device& _device) {
  } // RemoveDevice
  
  
}