/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland

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

#include "framebucketbase.h"

#include <cassert>

#include "core/base.h"
#include "core/logger.h"
#include "businterfacehandler.h"

namespace dss {

  //================================================== FrameBucketBase

  FrameBucketBase::FrameBucketBase(BusInterfaceHandler* _proxy, int _functionID, int _sourceID)
  : m_pProxy(_proxy),
    m_FunctionID(_functionID),
    m_SourceID(_sourceID)
  {
    assert(m_pProxy != NULL);
  } // ctor

  void FrameBucketBase::addToProxy() {
    Logger::getInstance()->log("Bucket: Registering for fid: " + intToString(m_FunctionID) + " sid: " + intToString(m_SourceID));
    m_pProxy->addFrameBucket(this);
  } // addToProxy

  void FrameBucketBase::removeFromProxyAndDelete(FrameBucketBase* _obj) {
    _obj->removeFromProxy();
    delete _obj;
  } // remove_from_proxy_and_delete

  void FrameBucketBase::removeFromProxy() {
    Logger::getInstance()->log("Bucket: Removing for fid: " + intToString(m_FunctionID) + " sid: " + intToString(m_SourceID));
    m_pProxy->removeFrameBucket(this);
  } // removeFromProxy


} // namespace dss
