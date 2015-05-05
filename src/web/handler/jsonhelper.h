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

  class JSONWriter;
  class DeviceReference;
  class Set;
  class Group;
  class Cluster;
  class Zone;
  class Apartment;

  void toJSON(const DeviceReference& _device, JSONWriter& _json);
  void toJSON(const Set& _set, JSONWriter& _json);
  void toJSON(boost::shared_ptr<const Group> _group, JSONWriter& _json);
  void toJSON(boost::shared_ptr<const Cluster> _cluster, JSONWriter& _json);
  void toJSON(Zone& _zone, JSONWriter& _json, bool _includeDevices = true);
  void toJSON(Apartment& _apartment, JSONWriter& _json);

} // namespace dss

#endif /* JSONHELPER_H_ */
