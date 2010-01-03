/*
    Copyright (c) 2010 digitalSTROM.org, Zurich, Switzerland
    Copyright (c) 2010 futureLAB AG, Winterthur, Switzerland

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

namespace dss {

  /** A Model event gets processed by the apartment asynchronously.
   * It consists of multiple integer parameter whose meanig is defined by ModelEvent::EventType
   */
  class ModelEvent {
  public:
    typedef enum { etCallSceneGroup,  /**< A group has changed the scene. */
                   etCallSceneDevice, /**< A device has changed the scene (only raised from the simulation at the moment). */
                   etNewDevice,       /**< A new device has been detected */
                   etModelDirty,      /**< A parameter that will be stored in \c apartment.xml has been changed. */
                   etDSLinkInterrupt,  /**< An interrupt has occured */
                   etNewModulator, /**< A new modulator has joined the bus */
                   etLostModulator, /**< We've lost a modulator on the bus */
                   etModulatorReady, /**< A modulator has completed its scanning cycle and is now ready */
                   etBusReady, /**< The bus transitioned into ready state */
                   etPowerConsumption, /**< Powerconsumption message happened */
                   etEnergyMeterValue, /**< Powerconsumption message happened */
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

} // namespace dss

#endif // MODELEVENT_H
