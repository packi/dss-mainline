/*
 *  modeljstests.cpp
 *  dSS
 *
 *  Created by Patrick Stählin on 4/23/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include "modeljs.h"

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/BriefTestProgressListener.h>
#include <cppunit/extensions/TestFactoryRegistry.h>

#undef CPPUNIT_ASSERT
#define CPPUNIT_ASSERT(condition) CPPUNIT_ASSERT_EQUAL(true, (condition))

using namespace dss;

class ModelTestJS : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(ModelTestJS);
  CPPUNIT_TEST(testBasics);
  CPPUNIT_TEST(testSets);
  CPPUNIT_TEST(testDevices);
  CPPUNIT_TEST_SUITE_END();
  
public:
  void setUp(void) {}
  void tearDown(void) {} 
  
  
protected:
  void testBasics(void) { 
    Apartment apt;
    apt.SetName("my apartment");

    
    ScriptEnvironment* env = new ScriptEnvironment();
    env->Initialize();
    ModelScriptContextExtension* ext = new ModelScriptContextExtension(apt);
    env->AddExtension(ext);
    
    ScriptContext* ctx = env->GetContext();
    ctx->LoadFromMemory("getName()");
    string name = ctx->Evaluate<string>();
    delete ctx;
    
    CPPUNIT_ASSERT_EQUAL(apt.GetName(), name);
    
    ctx = env->GetContext();
    ctx->LoadFromMemory("setName('hello'); getName()");
    
    name = ctx->Evaluate<string>();
    delete ctx;
    
    CPPUNIT_ASSERT_EQUAL(string("hello"), name);
    CPPUNIT_ASSERT_EQUAL(string("hello"), apt.GetName());
  } // testBasics
  
  void testSets() {
    Apartment apt;
    
    Device dev1 = apt.AllocateDevice(1);
    Device dev2 = apt.AllocateDevice(2);
    
    ScriptEnvironment* env = new ScriptEnvironment();
    env->Initialize();
    ModelScriptContextExtension* ext = new ModelScriptContextExtension(apt);
    env->AddExtension(ext);
    
    ScriptContext* ctx = env->GetContext();
    ctx->LoadFromMemory("var devs = getDevices(); devs.length()");
    int length = ctx->Evaluate<int>();
    delete ctx;
    
    CPPUNIT_ASSERT_EQUAL(2, length);

    ctx = env->GetContext();
    ctx->LoadFromMemory("var devs = getDevices(); var devs2 = getDevices(); devs.combine(devs2)");
    ctx->Evaluate<void>();
    delete ctx;

  } // testSets
  
  void testDevices() {
    Apartment apt;
    
    Device& dev1 = apt.AllocateDevice(1);
    dev1.SetName("dev1");
    Device& dev2 = apt.AllocateDevice(2);
    dev2.SetName("dev2");
    
    ScriptEnvironment* env = new ScriptEnvironment();
    env->Initialize();
    ModelScriptContextExtension* ext = new ModelScriptContextExtension(apt);
    env->AddExtension(ext);
    
    ScriptContext* ctx = env->GetContext();
    ctx->LoadFromMemory("var devs = getDevices();\n"
                        "var f = function(dev) { print(dev.name); }\n"
                        "devs.perform(f)\n");
    ctx->Evaluate<void>();
    delete ctx;
  }
};

CPPUNIT_TEST_SUITE_REGISTRATION(ModelTestJS);



