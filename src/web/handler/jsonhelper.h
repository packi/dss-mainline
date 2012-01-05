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


#ifndef JSONHELPER_H_
#define JSONHELPER_H_

#include <boost/shared_ptr.hpp>

namespace dss {

  class JSONObject;
  class JSONArrayBase;
  class DeviceReference;
  class Set;
  class Group;
  class Zone;
  class Apartment;

  boost::shared_ptr<JSONObject> toJSON(const DeviceReference& _device);
  boost::shared_ptr<JSONArrayBase> toJSON(const Set& _set);
  boost::shared_ptr<JSONObject> toJSON(boost::shared_ptr<const Group> _group);
  boost::shared_ptr<JSONObject> toJSON(Zone& _zone, bool _includeDevices = true);
  boost::shared_ptr<JSONObject> toJSON(Apartment& _apartment);

} // namespace dss

#endif /* JSONHELPER_H_ */
