/*
    Copyright (c) 2016 digitalSTROM.org, Zurich, Switzerland

    Author: Chritian Hitz <christian.hitz@digitalstorm.com>

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

#include "staterequesthandler.h"

#include <ds/log.h>

#include "src/model/apartment.h"
#include "src/model/state.h"
#include "src/handler/system_states.h"
#include "jsonhelper.h"

namespace dss {

  //=========================================== StateRequestHandler

  StateRequestHandler::StateRequestHandler(Apartment& _apartment)
  : m_Apartment(_apartment)
  { }

  WebServerResponse StateRequestHandler::jsonHandleRequest(const RestfulRequest& request, boost::shared_ptr<Session> session, const struct mg_connection* connection) {
    auto&& method = request.getMethod();
    if(method == "set") {
      return set(request);
    } else {
     DS_FAIL_REQUIRE("Unhandled function.", method);
    }
  }

  std::string StateRequestHandler::set(const RestfulRequest& request) {
      auto&& addon = request.tryGetParameter("addon").value_or(std::string());
      auto&& name = request.getRequiredParameter("name");
      auto&& value = request.getRequiredParameter("value");

      // White list of allowed system states
      // TODO(someday): remove
      if (addon.empty()) {
        if (name != StateName::HeatingSystem
          && name != StateName::HeatingSystemMode
          && name != StateName::HeatingModeControl) {
            DS_FAIL_REQUIRE("Not allowed or not existing system state", name);
        }
      }
      // TODO(soon): fail on invalid value
      m_Apartment.getState(addon, name)->setState(coJSON, value);
      return JSONWriter::success();
  }

} // namespace dss
