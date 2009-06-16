/*
 *  tests.cpp
 *  dSS
 *
 *  Created by Patrick Stählin on 4/11/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include "tests.h"

#include <cppunit/TestRunner.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/BriefTestProgressListener.h>
#include <cppunit/extensions/TestFactoryRegistry.h>


namespace dss {

  bool Tests::run() {
    //--- Create the event manager and test controller
    CPPUNIT_NS::TestResult controller;

    //--- Add a listener that colllects test result
    CPPUNIT_NS::TestResultCollector result;
    controller.addListener( &result );

    //--- Add a listener that print dots as test run.
    CPPUNIT_NS::BriefTestProgressListener progress;
    controller.addListener( &progress );

    //--- Add the top suite to the test runner
    CPPUNIT_NS::TestRunner runner;
    runner.addTest( CPPUNIT_NS::TestFactoryRegistry::getRegistry().makeTest() );
    runner.run( controller );

    return result.wasSuccessful();
  }

}
