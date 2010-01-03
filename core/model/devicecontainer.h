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

#ifndef DEVICECONTAINER_H
#define DEVICECONTAINER_H

#include <string>

namespace dss {

  class Set;

  /** A class derived from DeviceContainer can deliver a Set of its Devices */
  class DeviceContainer {
  private:
    std::string m_Name;
  public:
    /** Returns a set containing all devices of the container. */
    virtual Set getDevices() const = 0;

    /** Sets the name of the container. */
    virtual void setName(const std::string& _name);
    /** Returns the name of the container */
    const std::string& getName() const { return m_Name; };

    virtual ~DeviceContainer() {};
  }; // DeviceContainer

} // namespace dss

#endif // DEVICECONTAINER_H
