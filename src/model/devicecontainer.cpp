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

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "devicecontainer.h"

#include "src/dss.h"
#include "modelevent.h"
#include "apartment.h"
#include "modelmaintenance.h"
#include "src/stringconverter.h"

namespace dss {

  //================================================== DeviceContainer

  void DeviceContainer::setName(const std::string& _name) {
    if(m_Name != _name) {
      try {
        StringConverter st("UTF-8", "UTF-8");
        m_Name = st.convert(_name);
        if(DSS::hasInstance()) {
          DSS::getInstance()->getModelMaintenance().addModelEvent(new ModelEvent(ModelEvent::etModelDirty));
        }
      } catch (DSSException& e) {
        // could not convert name to UTF-8
      }
    }
  }

} // namespace dss
