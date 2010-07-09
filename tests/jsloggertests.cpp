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

#include <iostream>
#include <fstream>

#include <signal.h>

#include "core/base.h"
#include "core/scripting/scriptobject.h"
#include "core/scripting/jslogger.h"
#include "core/foreach.h"

using namespace std;
using namespace dss;

namespace fs = boost::filesystem;

BOOST_AUTO_TEST_SUITE(JSLogger)

BOOST_AUTO_TEST_CASE(testOneLoggerGetsCleanedUp) {
  EventQueue queue;
  EventRunner runner;
  EventInterpreter interpreter(NULL);

  interpreter.setEventQueue(&queue);
  interpreter.setEventRunner(&runner);
  EventInterpreterInternalRelay* relay = new EventInterpreterInternalRelay(&interpreter);
  interpreter.addPlugin(relay);

  interpreter.run();
  
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptLoggerExtension* ext = new ScriptLoggerExtension(getTempDir(), interpreter);
  env->addExtension(ext);

  fs::remove(getTempDir() + "blalog");
  fs::remove(getTempDir() + "blalog2");

  {
    boost::scoped_ptr<ScriptContext> ctx(env->getContext());
    ctx->evaluate<void>("var logger = new Logger('blalog');\n");
    BOOST_CHECK_EQUAL(ext->getNumberOfLoggers(), 1); 
  }
  
  BOOST_CHECK_EQUAL(ext->getNumberOfLoggers(), 0); 
  
  {
    boost::scoped_ptr<ScriptContext> ctx(env->getContext());
    ctx->evaluate<void>("var logger = new Logger('blalog');\n");
    BOOST_CHECK_EQUAL(ext->getNumberOfLoggers(), 1); 
    ctx->evaluate<void>("var logger2 = new Logger('blalog');\n");
    BOOST_CHECK_EQUAL(ext->getNumberOfLoggers(), 1); 
  }
  
  BOOST_CHECK_EQUAL(ext->getNumberOfLoggers(), 0); 

  {
    boost::scoped_ptr<ScriptContext> ctx(env->getContext());
    ctx->evaluate<void>("var logger = new Logger('blalog');\n");
    BOOST_CHECK_EQUAL(ext->getNumberOfLoggers(), 1); 
    ctx->evaluate<void>("var logger2 = new Logger('blalog2');\n");

    BOOST_CHECK_EQUAL(ext->getNumberOfLoggers(), 2); 
  }
  
  BOOST_CHECK_EQUAL(ext->getNumberOfLoggers(), 0);

  fs::remove(getTempDir() + "blalog");
  fs::remove(getTempDir() + "blalog2");

  queue.shutdown();
  interpreter.terminate();
  sleepMS(1500);
}


BOOST_AUTO_TEST_CASE(testLogger) {
  EventQueue queue;
  EventRunner runner;
  EventInterpreter interpreter(NULL);

  interpreter.setEventQueue(&queue);
  interpreter.setEventRunner(&runner);
  EventInterpreterInternalRelay* relay = new EventInterpreterInternalRelay(&interpreter);
  interpreter.addPlugin(relay);

  interpreter.run();
    
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptLoggerExtension* ext = new ScriptLoggerExtension(getTempDir(), interpreter);
  env->addExtension(ext);

  fs::remove(getTempDir() + "blalog");

  {
    boost::scoped_ptr<ScriptContext> ctx(env->getContext());
    ctx->evaluate<void>("var logger = new Logger('blalog');\n"
                        "logger.log('kraah');\n");
  }
  
  BOOST_CHECK(fs::exists(getTempDir() + "blalog"));


  std::string line;
  std::string text;


  if (fs::exists(getTempDir() + "blalog")) {
    ifstream blalog(std::string(getTempDir() + "blalog").c_str());
    if (blalog.is_open()) {
      while (!blalog.eof()) {
        getline(blalog, line);
        text = text + line;
      }
      blalog.close();
    }
  }

  BOOST_CHECK(text.find("kraah") != std::string::npos);

  fs::remove(getTempDir() + "blalog");

  queue.shutdown();
  interpreter.terminate();
  sleepMS(1500);
}

BOOST_AUTO_TEST_CASE(testLogrotate) {
  EventQueue queue;
  EventRunner runner;
  EventInterpreter interpreter(NULL);

  interpreter.setEventQueue(&queue);
  interpreter.setEventRunner(&runner);
  EventInterpreterInternalRelay* relay = new EventInterpreterInternalRelay(&interpreter);
  interpreter.addPlugin(relay);

  interpreter.run();
    
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptLoggerExtension* ext = new ScriptLoggerExtension(getTempDir(), interpreter);
  env->addExtension(ext);

  fs::remove(getTempDir() + "blalog");

  {
    boost::scoped_ptr<ScriptContext> ctx(env->getContext());
    ctx->evaluate<void>("var logger = new Logger('blalog');\n"
                        "logger.log('kraah');\n");
  }
  
  BOOST_CHECK(fs::exists(getTempDir() + "blalog"));

  fs::remove(getTempDir() + "blalog1");
  fs::copy_file(getTempDir() + "blalog", getTempDir() + "blalog1");
  fs::remove(getTempDir() + "blalog");

  boost::shared_ptr<Event> pEvent(new Event("SIGNAL"));
  pEvent->setProperty("signum", intToString(SIGUSR1));
  queue.pushEvent(pEvent);

  sleepMS(3000);

  {
    boost::scoped_ptr<ScriptContext> ctx(env->getContext());
    ctx->evaluate<void>("var logger = new Logger('blalog');\n"
                        "logger.log('kraah');\n");
  }
  
  BOOST_CHECK(fs::exists(getTempDir() + "blalog"));
 
  fs::remove(getTempDir() + "blalog");
  fs::remove(getTempDir() + "blalog1");

  queue.shutdown();
  interpreter.terminate();
  sleepMS(2500);
}

BOOST_AUTO_TEST_SUITE_END()

