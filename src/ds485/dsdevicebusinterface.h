/*
    Copyright (c) 2010 digitalSTROM.org, Zurich, Switzerland

    Authors: Patrick Staehlin, futureLAB AG <pstaehlin@futurelab.ch>
             Sergey 'Jin' Bostandzhyan <jin@dev.digitalstrom.org>

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

#ifndef DSDEVICEBUSINTERFACE_H_
#define DSDEVICEBUSINTERFACE_H_

#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#include "dsbusinterfaceobj.h"
#include "taskprocessor.h"
#include "src/businterface.h"

using boost::shared_ptr;

namespace dss {
  class Device;
  class DSMeter;

  class DSDeviceBusInterface : public DSBusInterfaceObj,
                               public DeviceBusInterface {
  public:
    class ConfigReaderHelper
    {
    public:
      ConfigReaderHelper(const std::string& _busConnection);
      ~ConfigReaderHelper();

      virtual void setup(boost::shared_ptr<Device> _device);

      uint16_t getDeviceConfigWord(uint8_t _configClass, uint8_t _configIndex) const;
      uint8_t getDeviceConfig(uint8_t _configClass, uint8_t _configIndex) const;
    protected:
      std::string m_busConnection;
      DsmApiHandle_t m_dsmApiHandle;
      devid_t m_deviceAdress;
      dsuid_t m_dsmId;
      boost::shared_ptr<DSMeter> m_dsm;
      int m_revisionID;
    };
    class OEMDataReader : public Task, public ConfigReaderHelper,
                          public boost::enable_shared_from_this< OEMDataReader > {
    public:
      OEMDataReader(const std::string& _busConnection);
      virtual ~OEMDataReader();
      virtual void run();
    };

    class TNYConfigReader : public Task, public ConfigReaderHelper,
                            public boost::enable_shared_from_this< TNYConfigReader > {
    public:
      TNYConfigReader(const std::string& _busConnection);
      virtual ~TNYConfigReader();
      virtual void run();
    };


    DSDeviceBusInterface()
    : DSBusInterfaceObj()
    { }

    virtual uint8_t getDeviceConfig(const Device& _device,
                                    uint8_t _configClass,
                                    uint8_t _configIndex);

    virtual uint16_t getDeviceConfigWord(const Device& _device,
                                       uint8_t _configClass,
                                       uint8_t _configIndex);

    virtual void setDeviceConfig(const Device& _device, uint8_t _configClass,
                                 uint8_t _configIndex, uint8_t _value);

    virtual void setDeviceButtonActiveGroup(const Device& _device,
                                            uint8_t _groupID);

    virtual void setDeviceProgMode(const Device& _device, uint8_t _modeId);

    virtual void setValue(const Device& _device, uint8_t _value);

    virtual void addGroup(const Device& _device, const int _groupId);
    virtual void removeGroup(const Device& _device, const int _groupId);

    virtual uint16_t getSensorValue(const Device& _device, const int _sensorIndex);
    virtual DeviceSensorValue_t getSensorValueEx(const Device& _device, const int _sensorIndex);

    virtual void lockOrUnlockDevice(const Device& _device, const bool _lock);

    virtual std::pair<uint8_t, uint16_t> getTransmissionQuality(const Device& _device);

    virtual void increaseDeviceOutputChannelValue(const Device& _device,
                                            uint8_t _channel);

    virtual void decreaseDeviceOutputChannelValue(const Device& _device,
                                            uint8_t _channel);

    virtual void stopDeviceOutputChannelValue(const Device& _device,
                                        uint8_t _channel);

    virtual uint16_t getDeviceOutputChannelValue(const Device& _device,
                                                 uint8_t _channel);

    virtual void setDeviceOutputChannelValue(const Device& _device,
                                             uint8_t _channel, uint8_t _size,
                                             uint16_t _value,
                                             bool _applyNow = true);

    virtual uint16_t getDeviceOutputChannelSceneValue(const Device& _device,
                                                      uint8_t _channel,
                                                      uint8_t _scene);

    virtual void setDeviceOutputChannelSceneValue(const Device& _device,
                                                  uint8_t _channel,
                                                  uint8_t _size, uint8_t _scene,
                                                  uint16_t _value);

    virtual uint16_t getDeviceOutputChannelSceneConfig(const Device& _device,
                                                       uint8_t _scene);

    virtual void setDeviceOutputChannelSceneConfig(const Device& _device,
                                                   uint8_t _scene,
                                                   uint16_t _value);
    virtual void setDeviceOutputChannelDontCareFlags(const Device& _device,
                                                     uint8_t _scene,
                                                     uint16_t _value);
    virtual uint16_t getDeviceOutputChannelDontCareFlags(const Device& _device,
                                                         uint8_t _scene);

    virtual void genericRequest(const Device& _device, const std::string& methodName,
                                const ::google::protobuf::RepeatedPtrField< ::vdcapi::PropertyElement >& params);
    virtual void setProperty(const Device& _device,
                             const ::google::protobuf::RepeatedPtrField< ::vdcapi::PropertyElement >& properties);
    virtual vdcapi::Message getProperty(const Device& _device,
        const ::google::protobuf::RepeatedPtrField< ::vdcapi::PropertyElement >& query);

  }; // DSDeviceBusInterface


} // namespace dss

#endif
