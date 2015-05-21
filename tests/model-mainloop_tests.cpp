/*
 *  Copyright (c) 2015 digitalSTROM.org, Zurich, Switzerland
 *
 *  Author: Andreas Fenkart, <andreas.fenkart@digitalstrom.com>
 *
 *  This file is part of digitalSTROM Server.
 *
 *  digitalSTROM Server is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  digitalSTROM Server is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with digitalSTROM Server. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#define BOOST_TEST_NO_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include "src/model/apartment.h"
#include "src/model/modelconst.h"
#include "src/model/modelmaintenance.h"
#include "tests/util/modelmaintenance-mockup.h"

using namespace dss;

BOOST_AUTO_TEST_SUITE(model_mainloop)

BOOST_AUTO_TEST_CASE(testHandleModelEvent) {
  ModelMaintenanceMock main;
  unsigned eventCountInit;

  eventCountInit = main.m_processedEvents;
  main.handleModelEvents();
  BOOST_CHECK_EQUAL(main.m_processedEvents, eventCountInit);
  BOOST_CHECK_EQUAL(main.handleModelEvents(), false);

  eventCountInit = main.m_processedEvents;
  main.addModelEvent(new ModelEvent(ModelEvent::etModelDirty));
  main.handleModelEvents();
  BOOST_CHECK_EQUAL(main.m_processedEvents - eventCountInit, 1);
  BOOST_CHECK_EQUAL(main.handleModelEvents(), false);

  // 2 dirty events are combined, only one is executed but both must be counted
  eventCountInit = main.m_processedEvents;
  main.addModelEvent(new ModelEvent(ModelEvent::etModelDirty));
  main.addModelEvent(new ModelEvent(ModelEvent::etModelDirty));
  main.handleModelEvents(); // single call increments by 2!
  BOOST_CHECK_EQUAL(main.m_processedEvents - eventCountInit, 2);
  BOOST_CHECK_EQUAL(main.handleModelEvents(), false);

  // same with heating state machine changes
  eventCountInit = main.m_processedEvents;
  main.addModelEvent(new ModelEvent(ModelEvent::etModelOperationModeChanged));
  main.addModelEvent(new ModelEvent(ModelEvent::etModelOperationModeChanged));
  main.handleModelEvents(); // single call increments by 2!
  BOOST_CHECK_EQUAL(main.m_processedEvents - eventCountInit, 2);
  BOOST_CHECK_EQUAL(main.handleModelEvents(), false);
}

BOOST_AUTO_TEST_CASE(testProcessedEventRollover) {
  ModelMaintenanceMock main;
  unsigned eventCountInit, queuedEvents;

  queuedEvents = eventCountInit = main.m_processedEvents;
  do {
    switch (queuedEvents % 3) {
    case 0:
      main.addModelEvent(new ModelEvent(ModelEvent::etModelDirty));
      break;
    case 1:
      main.addModelEvent(new ModelEvent(ModelEvent::etModelOperationModeChanged));
      break;
    case 2:
      main.addModelEvent(new ModelEvent(ModelEvent::etDummyEvent));
      break;
    }
    queuedEvents += 1;
  } while (queuedEvents > eventCountInit || (queuedEvents - eventCountInit) < 30);

  // the queue is modified by eraseModelEventsFromQueue. This behaviour can
  // only be tested if more than 1 event is queued. Hence need to queue first,
  // then process them. Otherwise there was only ever one element in the queue
  while (main.handleModelEvents());

  BOOST_CHECK(queuedEvents < eventCountInit); // rollover
  BOOST_CHECK_EQUAL(queuedEvents - eventCountInit, 30);
  BOOST_CHECK_EQUAL(main.m_processedEvents - eventCountInit, 30);
}

BOOST_AUTO_TEST_SUITE_END()
