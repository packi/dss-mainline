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
#include <boost/filesystem.hpp>

#include <boost/scoped_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/make_shared.hpp>

#include <iostream>
#include <fstream>

#include <signal.h>

#include "src/base.h"
#include "src/scripting/scriptobject.h"
#include "src/scripting/jslogger.h"
#include "src/foreach.h"
#include "src/event.h"
#include "internaleventrelaytarget.h"

using namespace std;
using namespace dss;

namespace fs = boost::filesystem;

BOOST_AUTO_TEST_SUITE(JSLogger)

BOOST_AUTO_TEST_CASE(testOneLoggerGetsCleanedUp) {
  EventInterpreter interpreter(NULL);
  EventQueue queue(&interpreter, 2);
  EventRunner runner(&interpreter);

  interpreter.initialize();
  interpreter.setEventQueue(&queue);
  interpreter.setEventRunner(&runner);
  EventInterpreterInternalRelay* relay = new EventInterpreterInternalRelay(&interpreter);
  interpreter.addPlugin(relay);

  interpreter.run();
  
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptLoggerExtension* ext = new ScriptLoggerExtension(getTempDir(), interpreter);
  env->addExtension(ext);

  fs::remove(getTempDir() + "blalog.log");
  fs::remove(getTempDir() + "blalog2.log");

  {
    boost::scoped_ptr<ScriptContext> ctx(env->getContext());
    ctx->evaluate<void>("var logger = new Logger('blalog.log');\n");
    BOOST_CHECK_EQUAL(ext->getNumberOfLoggers(), 1); 
  }

  BOOST_CHECK_EQUAL(ext->getNumberOfLoggers(), 0);

  {
    boost::scoped_ptr<ScriptContext> ctx(env->getContext());
    ctx->evaluate<void>("var logger = new Logger('blalog.log');\n");
    BOOST_CHECK_EQUAL(ext->getNumberOfLoggers(), 1); 
    ctx->evaluate<void>("var logger2 = new Logger('blalog.log');\n");
    BOOST_CHECK_EQUAL(ext->getNumberOfLoggers(), 1); 
  }

  BOOST_CHECK_EQUAL(ext->getNumberOfLoggers(), 0);

  {
    boost::scoped_ptr<ScriptContext> ctx(env->getContext());
    ctx->evaluate<void>("var logger = new Logger('blalog.log');\n");
    BOOST_CHECK_EQUAL(ext->getNumberOfLoggers(), 1); 
    ctx->evaluate<void>("var logger2 = new Logger('blalog2.log');\n");

    BOOST_CHECK_EQUAL(ext->getNumberOfLoggers(), 2); 
  }

  BOOST_CHECK_EQUAL(ext->getNumberOfLoggers(), 0);

  fs::remove(getTempDir() + "blalog.log");
  fs::remove(getTempDir() + "blalog2.log");

  queue.shutdown();
  interpreter.terminate();
  sleepMS(5);
}


BOOST_AUTO_TEST_CASE(testLogger) {
  EventInterpreter interpreter(NULL);
  EventQueue queue(&interpreter, 2);
  EventRunner runner(&interpreter);

  interpreter.initialize();
  interpreter.setEventQueue(&queue);
  interpreter.setEventRunner(&runner);
  EventInterpreterInternalRelay* relay = new EventInterpreterInternalRelay(&interpreter);
  interpreter.addPlugin(relay);

  interpreter.run();

  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptLoggerExtension* ext = new ScriptLoggerExtension(getTempDir(), interpreter);
  env->addExtension(ext);

  fs::remove(getTempDir() + "blalog.log");

  {
    boost::scoped_ptr<ScriptContext> ctx(env->getContext());
    ctx->evaluate<void>("var logger = new Logger('blalog.log');\n"
                        "logger.log('kraah');\n");
  }

  BOOST_CHECK(fs::exists(getTempDir() + "blalog.log"));


  std::string line;
  std::string text;


  if (fs::exists(getTempDir() + "blalog.log")) {
    ifstream blalog(std::string(getTempDir() + "blalog.log").c_str());
    if (blalog.is_open()) {
      while (!blalog.eof()) {
        getline(blalog, line);
        text = text + line;
      }
      blalog.close();
    }
  }

  BOOST_CHECK(text.find("kraah") != std::string::npos);

  fs::remove(getTempDir() + "blalog.log");

  queue.shutdown();
  interpreter.terminate();
  sleepMS(60);
}

BOOST_AUTO_TEST_CASE(testLogrotate) {
  EventInterpreter interpreter(NULL);
  EventQueue queue(&interpreter, 2);
  EventRunner runner(&interpreter);

  interpreter.initialize();
  interpreter.setEventQueue(&queue);
  interpreter.setEventRunner(&runner);
  EventInterpreterInternalRelay* relay = new EventInterpreterInternalRelay(&interpreter);
  interpreter.addPlugin(relay);

  interpreter.run();

  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptLoggerExtension* ext = new ScriptLoggerExtension(getTempDir(), interpreter);
  env->addExtension(ext);

  fs::remove(getTempDir() + "blalog.log");

  {
    boost::scoped_ptr<ScriptContext> ctx(env->getContext());
    ctx->evaluate<void>("var logger = new Logger('blalog.log');\n"
                        "logger.log('kraah');\n");
  }

  BOOST_CHECK(fs::exists(getTempDir() + "blalog.log"));

  fs::remove(getTempDir() + "blalog1.log");
  fs::copy_file(getTempDir() + "blalog.log", getTempDir() + "blalog1.log");
  fs::remove(getTempDir() + "blalog.log");

  boost::shared_ptr<Event> pEvent = boost::make_shared<Event>(EventName::Signal);
  pEvent->setProperty("signum", intToString(SIGUSR1));
  queue.pushEvent(pEvent);

  sleepMS(5);

  {
    boost::scoped_ptr<ScriptContext> ctx(env->getContext());
    ctx->evaluate<void>("var logger = new Logger('blalog.log');\n"
                        "logger.log('kraah');\n");
  }

  BOOST_CHECK(fs::exists(getTempDir() + "blalog.log"));

  fs::remove(getTempDir() + "blalog.log");
  fs::remove(getTempDir() + "blalog1.log");

  queue.shutdown();
  interpreter.terminate();
  sleepMS(5);
}

BOOST_AUTO_TEST_SUITE_END()

