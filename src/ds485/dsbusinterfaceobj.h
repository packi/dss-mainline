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

#ifndef DSBUSINTERFACEOBJ_H_
#define DSBUSINTERFACEOBJ_H_

#include <digitalSTROM/dsm-api-v2/dsm-api.h>
#include <digitalSTROM/dsm-api-v2/dsm-api-const.h>
#include <boost/thread/recursive_mutex.hpp>

namespace google {
  namespace protobuf {
    template <typename T>
    class RepeatedPtrField;
  }
}
namespace vdcapi {
  class PropertyElement;
  class Message;
}

namespace dss {

  class DSBusInterfaceObj {
  public:
    DSBusInterfaceObj();
    void setDSMApiHandle(DsmApiHandle_t _value);

  protected:
    static boost::recursive_mutex m_DSMApiHandleMutex;
    DsmApiHandle_t m_DSMApiHandle;

    vdcapi::Message getVdcProperty(const dsuid_t& _dsuid, const dsuid_t& _meterDsuid,
        const ::google::protobuf::RepeatedPtrField< ::vdcapi::PropertyElement >& query);
    void setVdcProperty(const dsuid_t& _dsuid, const dsuid_t& _meterDsuid,
          const ::google::protobuf::RepeatedPtrField< ::vdcapi::PropertyElement >& properties);
  };

} // namespace dss

#endif
