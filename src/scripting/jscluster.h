/*
    Copyright (c) 2011 digitalSTROM.org, Zurich, Switzerland

    Author: Remy Mahler, digitalstrom AG <remy.mahler@digitalstrom.com>

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

#ifndef _JSCLUSTER_INCLUDED
#define _JSCLUSTER_INCLUDED

#include "src/scripting/jshandler.h"

namespace dss {

  class Apartment;
  class Cluster;
  class Device;

  class ClusterScriptExtension : public ScriptExtension {
  public:
    ClusterScriptExtension(Apartment& _apartment);
    virtual ~ClusterScriptExtension();
    virtual void extendContext(ScriptContext& _context);

    Apartment& getApartment();

    JSBool busAddToGroup(JSContext* cx, boost::shared_ptr<Device> _device, int _clusterId);
    JSBool busRemoveFromGroup(JSContext* cx, boost::shared_ptr<Device> _device, int _clusterId);
    JSBool busUpdateCluster(JSContext* cx, boost::shared_ptr<Cluster> _cluster);
    JSBool busUpdateClusterLock(JSContext* cx, boost::shared_ptr<Cluster> _cluster);

  private:
    Apartment& m_Apartment;
  }; // ClusterScriptExtension
}
#endif // _JSCLUSTER_INCLUDED
