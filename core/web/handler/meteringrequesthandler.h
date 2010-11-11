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


#ifndef METERINGREQUESTHANDLER_H_
#define METERINGREQUESTHANDLER_H_

#include "core/web/webrequests.h"

namespace dss {
  
  class Metering;
  class Apartment;

  class MeteringRequestHandler : public WebServerRequestHandlerJSON {
  public:
    MeteringRequestHandler(Apartment& _apartment, Metering& _metering);
    virtual WebServerResponse jsonHandleRequest(const RestfulRequest& _request, boost::shared_ptr<Session> _session);
  private:
    Apartment& m_Apartment;
    Metering& m_Metering;

    boost::shared_ptr<JSONObject> getResolutions();
    boost::shared_ptr<JSONObject> getSeries();
    boost::shared_ptr<JSONObject> getValues(const RestfulRequest& _request);
    boost::shared_ptr<JSONObject> getLatest(const RestfulRequest& _request);

  }; // MeteringRequestHandler

} // namespace dss

#endif /* METERINGREQUESTHANDLER_H_ */
