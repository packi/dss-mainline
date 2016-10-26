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

//#include <algorithm>

#include "staterequesthandler.h"

#include "src/model/apartment.h"
#include "jsonhelper.h"

namespace dss {

  //=========================================== StateRequestHandler

  StateRequestHandler::StateRequestHandler(Apartment& _apartment)
  : m_Apartment(_apartment)
  { }

  WebServerResponse StateRequestHandler::jsonHandleRequest(const RestfulRequest& _request, boost::shared_ptr<Session> _session, const struct mg_connection* _connection) {
    std::string errorMessage;
    if(_request.getMethod() == "set") {
      std::string addon = _request.getParameter("addon");
      std::string name = _request.getParameter("name");
      std::string value = _request.getParameter("value");

      if (addon.empty()) {
        return JSONWriter::failure("Parameter 'addon' missing");
      }
      if (name.empty()) {
        return JSONWriter::failure("Parameter 'name' missing");
      }
      if (value.empty()) {
        return JSONWriter::failure("Parameter 'value' missing");
      }

      try {
        boost::shared_ptr<State> pState = m_Apartment.getState(StateType_Script, addon, name);
        pState->setState(coJSON, value);
      } catch (ItemNotFoundException& e) {
        try {
          m_Apartment.getNonScriptState(name); // will throw if not found
          return JSONWriter::failure(std::string("State ") + " state not writable from script");
        } catch (ItemNotFoundException& e) {
          // nope definitely doesn't exist
          return JSONWriter::failure(std::string("State ") + e.what() + " not found");
        }
      }

      return JSONWriter::success();
    } else {
      throw std::runtime_error("Unhandled function");
    }
  } // jsonHandleRequest

} // namespace dss
