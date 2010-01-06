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

#define BOOST_TEST_NO_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include "core/model/busrequest.h"
#include "core/ds485const.h"
#include "core/busrequestdispatcher.h"
#include "core/model/apartment.h"
#include "core/model/device.h"

using namespace dss;

BOOST_AUTO_TEST_SUITE(BusRequests)

class TestDispatcher : public dss::BusRequestDispatcher {
public:
  int requestCount() { return m_Requests.size(); }
  boost::shared_ptr<BusRequest> getBusRequest(int _count) {
    return m_Requests.at(_count);
  }

  virtual void dispatchRequest(boost::shared_ptr<BusRequest> _pRequest) {
    m_Requests.push_back(_pRequest);
  }
private:
  std::vector<boost::shared_ptr<BusRequest> > m_Requests;
};

BOOST_AUTO_TEST_CASE(testRequestReachesDispatcher) {
  Apartment apt(NULL);
  Device& d1 = apt.allocateDevice(dsid_t(0,1));
  TestDispatcher dispatcher;
  apt.setBusRequestDispatcher(&dispatcher);
  boost::shared_ptr<CallSceneCommandBusRequest> request(new CallSceneCommandBusRequest());
  request->setTarget(&d1);
  request->setSceneID(SceneOff);
  
  BOOST_CHECK_EQUAL(0, dispatcher.requestCount());
  apt.dispatchRequest(request);
  BOOST_CHECK_EQUAL(1, dispatcher.requestCount());
}

BOOST_AUTO_TEST_SUITE_END()
