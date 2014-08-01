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

#define BOOST_TEST_NO_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>

#include "src/event.h"
#include "src/eventsubscriptionsession.h"

using namespace dss;

BOOST_AUTO_TEST_SUITE(RestfulAPITests)

BOOST_AUTO_TEST_CASE(testSubscribeUnsubscribe) {
  dss::EventInterpreter interp(NULL);

  dss::EventSubscriptionSession_t subs(new dss::EventSubscriptionSession(interp, 0x7f));
  dss::EventSubscriptionSession_t subs2(new dss::EventSubscriptionSession(interp, 0x7f));
  std::vector<dss::EventSubscriptionSession_t> coll;

  BOOST_CHECK(std::find(coll.begin(), coll.end(), subs) == coll.end());
  coll.push_back(subs);
  coll.push_back(subs2);
  BOOST_CHECK(std::find(coll.begin(), coll.end(), subs) != coll.end());
  BOOST_CHECK(std::find(coll.begin(), coll.end(), subs2) != coll.end());

  coll.erase(std::remove(coll.begin(), coll.end(), subs), coll.end());
  BOOST_CHECK(std::find(coll.begin(), coll.end(), subs) == coll.end());
  BOOST_CHECK(std::find(coll.begin(), coll.end(), subs2) == coll.end());

  coll.push_back(subs);
  coll.push_back(subs);
  coll.push_back(subs);
  coll.push_back(subs);
  coll.push_back(subs);
  coll.erase(std::remove(coll.begin(), coll.end(), subs), coll.end());
  BOOST_CHECK(std::find(coll.begin(), coll.end(), subs) == coll.end());
  BOOST_CHECK(coll.empty());
}

BOOST_AUTO_TEST_CASE(testSubscribeUnsubscribeByTokenId) {
  dss::EventInterpreter interp(NULL);

  dss::EventSubscriptionSession_t subs(new dss::EventSubscriptionSession(interp, 0x7f));
  dss::EventSubscriptionSession_t subs2(new dss::EventSubscriptionSession(interp, 0x7f));
  dss::EventSubscriptionSession_t subs3(new dss::EventSubscriptionSession(interp, 0x17));
  std::vector<dss::EventSubscriptionSession_t> coll;

  coll.push_back(subs);
  coll.push_back(subs2);
  coll.erase(std::remove_if(coll.begin(), coll.end(),
                            dss::EventSubscriptionSessionSelectById(0x7f)), coll.end());
  BOOST_CHECK(coll.empty());

  coll.push_back(subs);
  coll.push_back(subs2);
  coll.push_back(subs3);
  coll.erase(std::remove_if(coll.begin(), coll.end(),
                            dss::EventSubscriptionSessionSelectById(0x7f)), coll.end());
  BOOST_CHECK(std::find(coll.begin(), coll.end(), subs3) != coll.end());
  BOOST_CHECK(coll.size() == 1);
}

BOOST_AUTO_TEST_SUITE_END()
