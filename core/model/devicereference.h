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


#ifndef DEVICEREFERENCE_H
#define DEVICEREFERENCE_H

#include <boost/shared_ptr.hpp>

#include "core/ds485types.h"
#include "deviceinterface.h"

namespace dss {

  class Apartment;
  class Device;

    /** Internal reference to a device.
   * A DeviceReference is virtually interchangable with a device. It is used in places
     where a reference to a device is needed.
   */
  class DeviceReference : public IDeviceInterface {
  private:
    dss_dsid_t m_DSID;
    const Apartment* m_Apartment;
  public:
    /** Copy constructor */
    DeviceReference(const DeviceReference& _copy);
    DeviceReference(const dss_dsid_t _dsid, const Apartment* _apartment);
    DeviceReference(boost::shared_ptr<const Device> _device, const Apartment* _apartment);
    virtual ~DeviceReference() {};

    /** Returns a reference to the referenced device
     * @note This accesses the apartment which has to be valid.
     */
    boost::shared_ptr<Device> getDevice();
    /** @copydoc getDevice() */
    boost::shared_ptr<const Device> getDevice() const;
    /** Returns the DSID of the referenced device */
    dss_dsid_t getDSID() const;

    /** Returns the function id.
     * @note This will lookup the device */
    int getFunctionID() const;

    /** Compares two device references.
     * Device references are considered equal if their DSID match. */
    bool operator==(const DeviceReference& _other) const {
      return m_DSID == _other.m_DSID;
    }

    /** Returns the name of the referenced device.
     * @note This will lookup the device. */
    std::string getName() const;

    virtual void setValue(uint8_t _value);

    virtual void increaseValue();
    virtual void decreaseValue();

    /** Returns wheter the device is turned on.
     * @note The detection is soly based on the last called scene. As soon as we've
     * got submetering data this should reflect the real state.
     */
    virtual bool isOn() const;

    virtual void callScene(const int _sceneNr, const bool _force);
    virtual void saveScene(const int _sceneNr);
    virtual void undoScene();

    virtual void nextScene();
    virtual void previousScene();

    virtual void blink();

    virtual unsigned long getPowerConsumption();
 }; // DeviceReference

} // namespace dss

#endif // DEVICEREFERENCE_H
