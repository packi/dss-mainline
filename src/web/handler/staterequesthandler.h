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

#pragma once

#include <ds/common.h>
#include "src/web/webrequests.h"

namespace dss {

  class Set;
  class Apartment;
  class ModelMaintenance;

  class StateRequestHandler : public WebServerRequestHandlerJSON {
  public:
    StateRequestHandler(Apartment& _apartment);
    WebServerResponse jsonHandleRequest(const RestfulRequest& request, boost::shared_ptr<Session> session,
        const struct mg_connection* connection) DS_OVERRIDE;

  private:
    Apartment& m_Apartment;

    std::string set(const RestfulRequest& request);
  }; // StateRequestHandler

} // namespace dss
