/*
 *  scriptstest.cpp
 *  dSS
 *
 *  Created by Patrick Stählin on 4/11/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include "scriptstest.h"

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/BriefTestProgressListener.h>
#include <cppunit/extensions/TestFactoryRegistry.h>


#include "../core/jshandler.h"

using namespace dss;

#undef CPPUNIT_ASSERT
#define CPPUNIT_ASSERT(condition) CPPUNIT_ASSERT_EQUAL(true, (condition))

class ScriptsTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(ScriptsTest);
  CPPUNIT_TEST(testSimpleScripts);
  CPPUNIT_TEST(testMultipleIterations);
//  CPPUNIT_TEST(testExceptionHandling);
  CPPUNIT_TEST_SUITE_END();
  
public:
  void setUp(void) {}
  void tearDown(void) {} 
  
protected:
  void testSimpleScripts(void) { 
    ScriptEnvironment env;
    env.Initialize();
    ScriptContext* ctx = env.GetContext();
    ctx->LoadFromMemory("x = 10; print(x); x = x * x;");
    double result = ctx->Evaluate<double>();
    CPPUNIT_ASSERT_EQUAL(result, 100.0);  
    delete ctx;
    
    ctx = env.GetContext();
    ctx->LoadFromFile("data/test.js");
    result = ctx->Evaluate<double>();
    CPPUNIT_ASSERT_EQUAL(result, 100.0);
    delete ctx;
    
    ctx = env.GetContext();
    ctx->LoadFromMemory("x = 'bla'; x = x + 'bla';");
    string sres = ctx->Evaluate<string>();
    CPPUNIT_ASSERT_EQUAL(sres, string("blabla"));
    delete ctx;
  }
  
  void testMultipleIterations() {
    ScriptEnvironment env;
    env.Initialize();
    for(int i = 0; i < 100; i++) {
      ScriptContext* ctx = env.GetContext();
      ctx->LoadFromFile("data/test3.js");
      ctx->Evaluate<void>();
      cout << ".";
      delete ctx;
    }
    cout << endl;
  }
  
  void testExceptionHandling(void) {
    ScriptEnvironment* env = new ScriptEnvironment();
    env->Initialize();
    ScriptContext* ctx = env->GetContext();
    ctx->LoadFromMemory("x = {}; x.what = 'bla'; x.toString = function() { return 'bla'; }; throw x; x = 10;");
    try {
      ctx->Evaluate<double>();
      CPPUNIT_ASSERT(false);
    } catch(const ScriptRuntimeException& _ex) {
      CPPUNIT_ASSERT_EQUAL(_ex.GetExceptionMessage(), string("bla"));
    }
  }
};

CPPUNIT_TEST_SUITE_REGISTRATION(ScriptsTest);
