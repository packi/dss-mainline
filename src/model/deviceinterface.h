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

#ifndef DEVICEINTERFACE_H
#define DEVICEINTERFACE_H

#include <stdint.h>
#include <digitalSTROM/dsuid.h>

#include "sceneaccess.h"
#include "modelconst.h"

namespace dss {

  typedef enum callOrigin {
    coUnknown = 0,
    coJSScripting = 1,
    coJSON = 2,
    // missing value here
    coSubscription = 4,
    // missing value here
    coTest = 6,
    coSystem = 7,
    coSystemBinaryInput = 8,
    coDsmApi = 9,
    coSystemStartup = 10,

    coLast = 11, // keep last
  } callOrigin_t;

  inline bool validOrigin(unsigned int origin)
  {
    return (origin < coLast && origin != 5 && origin != 3);
  }

  /** Interface to a single or multiple devices.
   */
  class IDeviceInterface {
  public:
    /** Turns the device on.
     *  This will invoke scene "max".
     */
    virtual void turnOn(const callOrigin_t _origin, const SceneAccessCategory _category);
    /** Turns the device off.
     * This will invoke scene "min"
     */
    virtual void turnOff(const callOrigin_t _origin, const SceneAccessCategory _category);

    /** Increases the main value (e.g. brightness) */
    virtual void increaseValue(const callOrigin_t _origin, const SceneAccessCategory _category) = 0;
    /** Decreases the main value (e.g. brightness) */
    virtual void decreaseValue(const callOrigin_t _origin, const SceneAccessCategory _category) = 0;

    /** Sets the output value */
    virtual void setValue(const callOrigin_t _origin, const SceneAccessCategory _category, const uint8_t _value, const std::string _token) = 0;
    /** Sets the scene on the device.
     * The output value will be set according to the scene lookup table in the device.
     */
    virtual void callScene(const callOrigin_t _origin, const SceneAccessCategory _category, const int _sceneNr, const std::string _token, const bool _force = false) = 0;
    /** Sets the min Scene on the device.
     * The output value will be set according to the scene lookup table in the device.
     */
    virtual void callSceneMin(const callOrigin_t _origin, const SceneAccessCategory _category, const int _sceneNr, const std::string _token) = 0;
    /** Stores the current output value into the scene lookup table.
     * The next time scene _sceneNr gets called the output will be set according to the lookup table.
     */
    virtual void saveScene(const callOrigin_t _origin, const int _sceneNr, const std::string _token) = 0;
    /** Restores the last scene value if identical to _sceneNr */
    virtual void undoScene(const callOrigin_t _origin, const SceneAccessCategory _category, const int _sceneNr, const std::string _token) = 0;
    /** Restores the last scene value */
    virtual void undoSceneLast(const callOrigin_t _origin, const SceneAccessCategory _category, const std::string _token) = 0;

    /** Returns the consumption in mW */
    virtual unsigned long getPowerConsumption() = 0;

    /** Calls the next scene according to the last called scene.
     * @see dss::SceneHelper::getNextScene
     */
    virtual void nextScene(const callOrigin_t _origin, const SceneAccessCategory _category) = 0;
    /** Calls the previos scene according to the last called scene.
     * @see dss::SceneHelper::getPreviousScene
     */
    virtual void previousScene(const callOrigin_t _origin, const SceneAccessCategory _category) = 0;

    virtual void blink(const callOrigin_t _origin, const SceneAccessCategory _category, const std::string _token) = 0;

    virtual void increaseOutputChannelValue(const callOrigin_t _origin, const SceneAccessCategory _category, const uint8_t channel, const std::string _token) = 0;
    virtual void decreaseOutputChannelValue(const callOrigin_t _origin, const SceneAccessCategory _category, const uint8_t channel, const std::string _token) = 0;
    virtual void stopOutputChannelValue(const callOrigin_t _origin, const SceneAccessCategory _category, const uint8_t channel, const std::string _token) = 0;

    virtual void pushSensor(const callOrigin_t _origin, const SceneAccessCategory _category, dsuid_t _sourceID, SensorType _sensorType, double _sensorValueFloat, const std::string _token) = 0;

    virtual ~IDeviceInterface() {};
  }; // IDeviceInterface

} // namespace dss
#endif // DEVICEINTERFACE_H
