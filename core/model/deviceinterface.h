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

namespace dss {

  /** Interface to a single or multiple devices.
   */
  class IDeviceInterface {
  public:
    /** Turns the device on.
     *  This will invoke scene "max".
     */
    virtual void turnOn();
    /** Turns the device off.
     * This will invoke scene "min"
     */
    virtual void turnOff();

    /** Increases the main value (e.g. brightness) */
    virtual void increaseValue() = 0;
    /** Decreases the main value (e.g. brightness) */
    virtual void decreaseValue() = 0;

    /** Starts dimming the given parameter.
     * If _directionUp is true, the value gets increased over time. Else its getting decreased.
     */
    virtual void startDim(const bool _directionUp) = 0;
    /** Stops the dimming */
    virtual void endDim() = 0;
    /** Sets the value of the given parameter */
    virtual void setValue(const double _value) = 0;


    /** Sets the scene on the device.
     * The output value will be set according to the scene lookup table in the device.
     */
    virtual void callScene(const int _sceneNr) = 0;
    /** Stores the current output value into the scene lookup table.
     * The next time scene _sceneNr gets called the output will be set according to the lookup table.
     */
    virtual void saveScene(const int _sceneNr) = 0;
    /** Restores the last set value of _sceneNr
     */
    virtual void undoScene() = 0;

    /** Returns the consumption in mW */
    virtual unsigned long getPowerConsumption() = 0;

    /** Calls the next scene according to the last called scene.
     * @see dss::SceneHelper::getNextScene
     */
    virtual void nextScene() = 0;
    /** Calls the previos scene according to the last called scene.
     * @see dss::SceneHelper::getPreviousScene
     */
    virtual void previousScene() = 0;

    virtual void blink() = 0;

    virtual ~IDeviceInterface() {};
  }; // IDeviceInterface

} // namespace dss
#endif // DEVICEINTERFACE_H
