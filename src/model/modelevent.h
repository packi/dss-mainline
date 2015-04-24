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


#ifndef MODELEVENT_H
#define MODELEVENT_H

#include <vector>
#include <boost/shared_ptr.hpp>

#include "src/ds485types.h"

namespace dss {

  /** A Model event gets processed by the apartment asynchronously.
   * It consists of multiple integer parameter whose meanig is defined by ModelEvent::EventType
   */
  class ModelEvent {
  public:
    typedef enum { etCallSceneGroup,  /**< A group has changed the scene. */
                    etUndoSceneGroup,  /**< An undo scene command for a group. */
                    etBlinkGroup,  /**< A group was blinked. */
                    etCallSceneDevice, /**< A device has changed the scene */
                    etUndoSceneDevice, /**< A device undo scene command */
                    etBlinkDevice, /**< A device was blinked */
                    etCallSceneDeviceLocal, /**< A device has changed the status locally */
                    etButtonClickDevice, /** < A button click on a device not handled by the dSMeter */
                    etNewDevice,       /**< A new device has been detected */
                    etLostDevice,       /**< A device became inactive */
                    etDeviceChanged,   /**< The device needs to be rescanned */
                    etDeviceConfigChanged, /**< The devices config got changed */
                    etModelDirty,      /**< A parameter that will be stored in \c apartment.xml has been changed. */
                    etLostDSMeter, /**< We've lost a dsMeter on the bus */
                    etDSMeterReady, /**< A dsMeter has completed its scanning cycle and is now ready */
                    etBusReady, /**< The bus transitioned into ready state */
                    etBusDown, /**< The bus transitioned into disconnected state */
                    etMeteringValues, /**< Metering values arrived */
                    etDS485DeviceDiscovered, /**< A new device has been discovered on the bus */
                    etZoneSensorValue, /**< A zone sensor value has changed */
                    etDeviceSensorEvent, /**< A device has sent a sensor event */
                    etDeviceSensorValue, /**< A device has sent a new sensor value */
                    etDeviceBinaryStateEvent, /**< A device has sent a new binary input state */
                    etDeviceEANReady, /** OEM data has finished reading out from device */
                    etDeviceOEMDataReady, /** OEM data has been retrieved from webservice */
                    etControllerState, /** State of the controlling device or algorithm changed */
                    etControllerConfig, /** Configuration of the controlling device or algorithm changed */
                    etControllerValues, /** Operation Values of the controlling device or algorithm changed */
                    etModelOperationModeChanged, /**< A parameter that will be stored in \c apartment.xml has been changed, but the cloud service should not be informed about it */
                    etClusterConfigLock, /** config lock of a cluster was changed */
                    etClusterLockedScenes, /** locked scenes of a cluster was changed */
                 } EventType;
  private:
    EventType m_EventType;
    std::vector<int> m_Parameter;
    boost::shared_ptr<void> m_SingleObjectParameter;
    std::string m_SingleStringParameter; // won't add a vector/list until
                                         // we really need more string
                                         // parameters which is not yet the
                                         // case
  public:
    /** Constructs a ModelEvent with the given EventType. */
    ModelEvent(EventType _type)
    : m_EventType(_type)
    {}
    
    virtual ~ModelEvent() { }

    /** Adds an integer parameter. */
    void addParameter(const int _param) { m_Parameter.push_back(_param); }
    /** Returns the parameter at _index.
     * @note Check getParameterCount to check the bounds. */
    int getParameter(const int _index) const { return m_Parameter.at(_index); }
    /** Returns the parameter count. */
    int getParameterCount() const { return m_Parameter.size(); }
    /** Returns the type of the event. */
    EventType getEventType() { return m_EventType; }

    void setSingleStringParameter(const std::string _param) { m_SingleStringParameter = _param; }
    std::string getSingleStringParameter() const { return m_SingleStringParameter; }

    void setSingleObjectParameter(const boost::shared_ptr<void> _param) { m_SingleObjectParameter = _param; }
    boost::shared_ptr<void> getSingleObjectParameter() const { return m_SingleObjectParameter; }
  }; // ModelEvent

  // TODO: use boost::any for values and remove this class
  class ModelEventWithDSID : public ModelEvent {
  public:
    ModelEventWithDSID(EventType _type, const dsuid_t& _dsid) 
    : ModelEvent(_type),
      m_DSID(_dsid)
    { }
    
    const dsuid_t& getDSID() { return m_DSID; }
  private:
    dsuid_t m_DSID;
  }; // ModelEventWithDSID

  class ModelEventWithStrings : public ModelEventWithDSID {
  public:
    ModelEventWithStrings(EventType _type, const dsuid_t& _dsid)
      : ModelEventWithDSID(_type, _dsid)
    { }

    void addStringParameter(const std::string& _param) { m_StringParameter.push_back(_param); }
    const std::string& getStringParameter(const int _index) const { return m_StringParameter.at(_index); }
    int getStringParameterCount() const { return m_StringParameter.size(); }
  private:
    std::vector<std::string> m_StringParameter;
  };

} // namespace dss

#endif // MODELEVENT_H
