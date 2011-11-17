/*
    Copyright (c) 2009,2011 digitalSTROM.org, Zurich, Switzerland

    Author: Patrick Staehlin, futureLAB AG <pstaehlin@futurelab.ch>,
            Michael Tross, aizo GmbH <michael.tross@aizo.com>

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
#include <boost/shared_ptr.hpp>

#include <string>

#include "core/scripting/jshandler.h"
#include "core/scripting/scriptobject.h"

using namespace dss;

BOOST_AUTO_TEST_SUITE(JSHandler)

BOOST_AUTO_TEST_CASE(testSimpleObject) {
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();

  boost::shared_ptr<ScriptContext> ctx(env->getContext());
  boost::shared_ptr<ScriptObject> obj;

  {
    JSContextThread req(ctx);
    obj.reset(new ScriptObject(*ctx, &ctx->getRootObject()));
    obj->setProperty("testing", 1);
    BOOST_CHECK_EQUAL(obj->getProperty<int>("testing"), 1);
    ctx->getRootObject().setProperty("obj", obj.get());
  }

  BOOST_CHECK_EQUAL(ctx->evaluate<int>("obj.testing"), 1);
  {
    JSContextThread req(ctx);
    obj->setProperty("testing", 0);
  }
  BOOST_CHECK_EQUAL(ctx->evaluate<int>("obj.testing"), 0);

  {
    JSContextThread req(ctx);
    obj->setProperty("testing2", "test");
    BOOST_CHECK_EQUAL(obj->getProperty<std::string>("testing2"), "test");
  }
  BOOST_CHECK_EQUAL(ctx->evaluate<std::string>("obj.testing2"), "test");
} // testSimpleObject

BOOST_AUTO_TEST_CASE(testCallingFunctions) {
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();

  boost::shared_ptr<ScriptContext> ctx(env->getContext());
  jsval res = ctx->doEvaluate("dev = { func: function(a,b,c) { return { e: a, f: b, g: c }; } }; dev");
  {
    JSContextThread req(ctx);
    BOOST_ASSERT(JSVAL_IS_OBJECT(res));

    ScriptObject obj(JSVAL_TO_OBJECT(res), *ctx);

    ScriptFunctionParameterList list(*ctx);
    list.add<const std::string&>(std::string("testing"));
    list.add<int>(1);
    list.add<bool>(false);

    res = obj.doCallFunctionByName("func", list);

    BOOST_ASSERT(JSVAL_IS_OBJECT(res));
    ScriptObject resObj(JSVAL_TO_OBJECT(res), *ctx);
    BOOST_CHECK_EQUAL(resObj.getProperty<std::string>("e"), "testing");
    BOOST_CHECK_EQUAL(resObj.getProperty<int>("f"), 1);
    BOOST_CHECK_EQUAL(resObj.getProperty<bool>("g"), false);
  }
}

BOOST_AUTO_TEST_CASE(testLongerScript) {
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();

  boost::shared_ptr<ScriptContext> ctx(env->getContext());
  ctx->evaluate<void>("print('dummy.js, running');\n"
                      "\n"
                      "var pi = 3.14;\n"
                      "var r = 5;\n"
                      "var x = 1984;\n"
                      "\n"
                      "var area = pi * r * r;\n"
                      "var division = x / r;\n"
                      "\n"
                      "print('area: ' + area);\n"
                      "print('divison: ' + division);");
} // testLongerScript

BOOST_AUTO_TEST_CASE(testNonexistingScriptRaisesException) {
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();

  boost::shared_ptr<ScriptContext> ctx(env->getContext());
  BOOST_CHECK_THROW(ctx->evaluateScript<void>("idontexistandneverwill.js"), ScriptException);
} // testNonexistingScriptRaisesException

BOOST_AUTO_TEST_CASE(testSetTimeoutZeroDelay) {
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();

  boost::shared_ptr<ScriptContext> ctx(env->getContext());
  ctx->evaluate<void>("var result = false; setTimeout(function() {  result = true; }, 0);");
  while(ctx->hasAttachedObjects()) {
    sleepMS(1);
  }
  JSContextThread thread(ctx);
  BOOST_CHECK_EQUAL(ctx->getRootObject().getProperty<bool>("result"), true);
} // testSetTimeoutZeroDelay

BOOST_AUTO_TEST_CASE(testSetTimeoutNormalDelay) {
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();

  boost::shared_ptr<ScriptContext> ctx(env->getContext());
  ctx->evaluate<void>("var result = false; setTimeout(function() {  result = true; }, 10);");
  while(ctx->hasAttachedObjects()) {
    sleepMS(1);
  }
  //boost::mutex::scoped_lock lock = ctx->getLock();
  JSContextThread req(ctx);
  BOOST_CHECK_EQUAL(ctx->getRootObject().getProperty<bool>("result"), true);
} // testSetTimeoutNormalDelay

BOOST_AUTO_TEST_CASE(testSetTimeoutInvalidFunction) {
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();

  boost::shared_ptr<ScriptContext> ctx(env->getContext());
  ctx->evaluate<void>("var a = 1; var result = false; setTimeout(a, 0);");
  while(ctx->hasAttachedObjects()) {
    sleepMS(1);
  }
  JSContextThread req(ctx);
  BOOST_CHECK_EQUAL(ctx->getRootObject().getProperty<bool>("result"), false);
} // testSetTimeoutInvalidFunction

BOOST_AUTO_TEST_CASE(testSetTimeoutGC) {
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();

  boost::shared_ptr<ScriptContext> ctx(env->getContext());
  ctx->evaluate<void>("var result = false;\n"
                      "var obj = { func: function() { result = true; print('timeout active'); } };"
                      "setTimeout(function() { setTimeout(obj.func, 0); }, 3000);"
                      "print('timeout scheduled');"
      );
  while(ctx->hasAttachedObjects()) {
    sleepMS(1);
  }
  JSContextThread req(ctx);
  BOOST_CHECK_EQUAL(ctx->getRootObject().getProperty<bool>("result"), true);
}

BOOST_AUTO_TEST_CASE(testSetTimeoutIsStoppable) {
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();

  boost::shared_ptr<ScriptContext> ctx(env->getContext());
  ctx->evaluate<void>("result = true;\n"
                      "setTimeout(function() { result = false; }, 100000);");
  BOOST_CHECK_EQUAL(ctx->getAttachedObjectsCount(), 1);
  ctx->stop();

  // check that we're terminating soon-ish after issuing the stop command
  for(int i = 0; i < 21; i++) {
    if(ctx->getAttachedObjectsCount() == 0) {
      break;
    }
    sleepMS(25);
  } 
  BOOST_CHECK_EQUAL(ctx->getAttachedObjectsCount(), 0);
}

BOOST_AUTO_TEST_SUITE_END()
