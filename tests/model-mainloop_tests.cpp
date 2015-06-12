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

#include <boost/thread/thread.hpp>

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

  // dirty events were previously combined, which led to jumps in the processed
  // event counter
  eventCountInit = main.m_processedEvents;
  main.addModelEvent(new ModelEvent(ModelEvent::etModelDirty));
  main.addModelEvent(new ModelEvent(ModelEvent::etModelDirty));
  main.handleModelEvents();
  BOOST_CHECK_EQUAL(main.m_processedEvents - eventCountInit, 1);
  BOOST_CHECK_EQUAL(main.handleModelEvents(), true);

  // same with heating state machine changes
  eventCountInit = main.m_processedEvents;
  main.addModelEvent(new ModelEvent(ModelEvent::etModelOperationModeChanged));
  main.addModelEvent(new ModelEvent(ModelEvent::etModelOperationModeChanged));
  main.handleModelEvents();
  BOOST_CHECK_EQUAL(main.m_processedEvents - eventCountInit, 1);
  BOOST_CHECK_EQUAL(main.handleModelEvents(), true);
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
  int ct = 0;
  while (main.handleModelEvents()) { ct++; };

  BOOST_CHECK(queuedEvents < eventCountInit); // counter wrapped
  BOOST_CHECK_EQUAL(main.m_processedEvents, queuedEvents);
  BOOST_CHECK_EQUAL(main.m_processedEvents - eventCountInit, ct);
}

BOOST_AUTO_TEST_CASE(testEraseBreaksEventCounter) {
  //
  // why is erasing events from the queue evil?
  //
  // fig1
  // queue = o o o d <o'> | d d d d:
  // ct    = 1 2 3 4  5     6 7 8 9
  // d = dirty, o = other event
  //
  // We want to be sure o' is processed before continuing hence
  // we wait until the processed counter is >= 5.
  //
  // solution1: counting erased events
  // when procesessing event 4, we also remove events 6, 7, 8, 9 see fig1 hence
  // the processed counter increases to 10, o' not yet processed
  //
  // fig2
  // queue = o o o d <o'> | d d d d o:
  // ct    = 1 2 3    4             5
  //
  // solution2: not counting erased events
  // computing the index when o' will be procesed, is more difficult. We
  // we have maintain a list of events that must not be counted.
  // maintaining that list will be error prone
  //
  // fig3
  // queue = o o o d1 <o'> | d d d d:
  // ct    = 1 2 3 4   5     6 7 8 9
  //
  // solution3(adopted): erase no events, but delay writing apartment.xml
  // Writing apartment.xml to frequently is the intent why erasing events from
  // the queue was introduced, that is solved by 30s delay. Downside is,
  // that we expand the period we are vulnerable to power-fails or crashes
  // -- use logging journal
  //
  // A SECOND PROBLEM is notifying observers about model changes.
  // These must not be delayed by 30s!
  // When d1 is processed, see fig3, we signal our observers, but mask
  // such notifications for dirty events already queued (@6,7,8,9)
  // The reason is that modifications happen synchronously from the caller,
  // while only config write happens asynchronously.
  // E.g onRemoveDevice directly removes the device from the model
  // held in memory, but spawns etModelDirty to also save it persistently.
  // Hence all etModelDirty events already queued hold no new information
  // to be siganlled once they are processed
  // EXCEPTION: etDeviceDirty restores some model invariants, then
  // schedules etModelDirty when done
  //
  ModelMaintenanceMock main;
  unsigned eventCountInit = main.m_processedEvents;

  // we want to be sure the etDummyEvent is executed
  main.addModelEvent(new ModelEvent(ModelEvent::etModelDirty));
  main.addModelEvent(new ModelEvent(ModelEvent::etDummyEvent)); // o'
  main.addModelEvent(new ModelEvent(ModelEvent::etModelDirty));

  int ct = 0;
  while (main.handleModelEvents()) { ct++; };
  BOOST_CHECK_EQUAL(ct, 3);
  BOOST_CHECK_EQUAL(main.m_processedEvents - eventCountInit, 3);
}

BOOST_AUTO_TEST_CASE(testIndexOfSyncState) {
  ModelMaintenanceMock main;
  unsigned eventCountInit = main.m_processedEvents;

  main.addModelEvent(new ModelEvent(ModelEvent::etDummyEvent)); // 1
  main.addModelEvent(new ModelEvent(ModelEvent::etDummyEvent));
  main.addModelEvent(new ModelEvent(ModelEvent::etDummyEvent));
  main.addModelEvent(new ModelEvent(ModelEvent::etModelDirty)); // 4
  main.addModelEvent(new ModelEvent(ModelEvent::etDummyEvent));
  main.addModelEvent(new ModelEvent(ModelEvent::etDummyEvent));
  main.addModelEvent(new ModelEvent(ModelEvent::etModelDirty)); // 7
  main.addModelEvent(new ModelEvent(ModelEvent::etDummyEvent));
  main.addModelEvent(new ModelEvent(ModelEvent::etDummyEvent));
  main.addModelEvent(new ModelEvent(ModelEvent::etDummyEvent)); // 10

  BOOST_CHECK_EQUAL(main.indexOfNextSyncState(), 7 + eventCountInit);

  // barrier will not let us pass until event 7
  BOOST_CHECK_EQUAL(main.pendingChangesBarrier(0), false);
  for (int i = 1; i < 7; i++) {
    main.handleModelEvents();
    BOOST_CHECK_EQUAL(main.pendingChangesBarrier(0), false);
  }

  main.handleModelEvents();
  BOOST_CHECK_EQUAL(main.pendingChangesBarrier(0), true);

  // no more dirty events on the queue
  // -> barrier let's us pass immediately:
  BOOST_CHECK_EQUAL(main.indexOfNextSyncState(), main.m_processedEvents);
  BOOST_CHECK_EQUAL(main.pendingChangesBarrier(0), true);
}

void wait_barrier(ModelMaintenanceMock &m, bool &ret)
{
  ret = m.pendingChangesBarrier(60);
}

BOOST_AUTO_TEST_CASE(testChangesBarier) {
  ModelMaintenanceMock main;

  main.addModelEvent(new ModelEvent(ModelEvent::etDummyEvent)); // 1
  main.addModelEvent(new ModelEvent(ModelEvent::etDummyEvent));
  main.addModelEvent(new ModelEvent(ModelEvent::etDummyEvent));
  main.addModelEvent(new ModelEvent(ModelEvent::etModelDirty)); // 4
  main.addModelEvent(new ModelEvent(ModelEvent::etDummyEvent));
  main.addModelEvent(new ModelEvent(ModelEvent::etDummyEvent));
  main.addModelEvent(new ModelEvent(ModelEvent::etModelDirty)); // 7
  main.addModelEvent(new ModelEvent(ModelEvent::etDummyEvent));
  main.addModelEvent(new ModelEvent(ModelEvent::etDummyEvent));
  main.addModelEvent(new ModelEvent(ModelEvent::etDummyEvent)); // 10

  unsigned eventCountInit = main.m_processedEvents;
  unsigned syncState = main.indexOfNextSyncState();
  BOOST_CHECK_EQUAL(syncState, 7 + eventCountInit);

  bool ret = false;
  boost::thread t(wait_barrier, boost::ref(main), boost::ref(ret));
  boost::this_thread::sleep(boost::posix_time::milliseconds(5));

  // these must be ignored, they arrived after we started waiting
  // we only wait till #7
  for (int i = 0; i < 10; i++) {
    main.addModelEvent(new ModelEvent(ModelEvent::etDummyEvent));
  }
  main.addModelEvent(new ModelEvent(ModelEvent::etModelDirty));

  while (ret == false && main.handleModelEvents()) {
    boost::this_thread::sleep(boost::posix_time::milliseconds(5));
  }
  t.join();

  BOOST_CHECK_EQUAL(ret, true);
  BOOST_CHECK_EQUAL(main.m_processedEvents, syncState);

  // new dirty events arrived, barrier moved on
  // -> we would have to wait again
  BOOST_CHECK_NE(main.indexOfNextSyncState(), main.m_processedEvents);
  BOOST_CHECK_EQUAL(main.pendingChangesBarrier(0), false);
}

BOOST_AUTO_TEST_SUITE_END()
