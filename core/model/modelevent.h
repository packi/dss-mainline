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

#include "core/ds485types.h"

namespace dss {

  /** A Model event gets processed by the apartment asynchronously.
   * It consists of multiple integer parameter whose meanig is defined by ModelEvent::EventType
   */
  class ModelEvent {
  public:
    typedef enum { etCallSceneGroup,  /**< A group has changed the scene. */
                   etCallSceneDevice, /**< A device has changed the scene */
                   etNewDevice,       /**< A new device has been detected */
                   etLostDevice,       /**< A device became inactive */
                   etDeviceChanged,   /**< The device needs to be rescanned */
                   etDeviceConfigChanged, /**< The devices config got changed */
                   etModelDirty,      /**< A parameter that will be stored in \c apartment.xml has been changed. */
                   etLostDSMeter, /**< We've lost a dsMeter on the bus */
                   etDSMeterReady, /**< A dsMeter has completed its scanning cycle and is now ready */
                   etBusReady, /**< The bus transitioned into ready state */
                   etMeteringValues, /**< Metering values arrived */
                   etDS485DeviceDiscovered, /**< A new device has been discovered on the bus */
                 } EventType;
  private:
    EventType m_EventType;
    std::vector<int> m_Parameter;
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
  }; // ModelEvent

  // TODO: use boost::any for values and remove this class
  class ModelEventWithDSID : public ModelEvent {
  public:
    ModelEventWithDSID(EventType _type, const dss_dsid_t& _dsid) 
    : ModelEvent(_type),
      m_DSID(_dsid)
    { }
    
    const dss_dsid_t& getDSID() { return m_DSID; }
  private:
    dss_dsid_t m_DSID;
  }; // ModelEventWithDSID

} // namespace dss

#endif // MODELEVENT_H
