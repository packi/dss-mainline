/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland
    Copyright (c) 2009 futureLAB AG, Winterthur, Switzerland

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

#include "../core/jshandler.h"

using namespace dss;

BOOST_AUTO_TEST_SUITE(JSHandler)

BOOST_AUTO_TEST_CASE(testSimpleObject) {
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  ScriptObject obj(*ctx, NULL);
  obj.setProperty("testing", 1);
  BOOST_CHECK_EQUAL(obj.getProperty<int>("testing"), 1);
  ctx->getRootObject().setProperty("obj", &obj);

  BOOST_CHECK_EQUAL(ctx->evaluate<int>("obj.testing"), 1);
  obj.setProperty("testing", 0);
  BOOST_CHECK_EQUAL(ctx->evaluate<int>("obj.testing"), 0);

  obj.setProperty("testing2", "test");
  BOOST_CHECK_EQUAL(obj.getProperty<std::string>("testing2"), "test");
  BOOST_CHECK_EQUAL(ctx->evaluate<std::string>("obj.testing2"), "test");
} // testSimpleObject

BOOST_AUTO_TEST_CASE(testCallingFunctions) {
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  jsval res = ctx->evaluate<jsval>("dev = { func: function(a,b,c) { return { e: a, f: b, g: c }; } }; dev");

  BOOST_ASSERT(JSVAL_IS_OBJECT(res));

  ScriptObject obj(JSVAL_TO_OBJECT(res), *ctx);

  ScriptFunctionParameterList list(*ctx);
  list.add<const std::string&>(std::string("testing"));
  list.add<int>(1);
  list.add<bool>(false);

  res = obj.callFunctionByName<jsval>("func", list);

  BOOST_ASSERT(JSVAL_IS_OBJECT(res));
  ScriptObject resObj(JSVAL_TO_OBJECT(res), *ctx);
  BOOST_CHECK_EQUAL(resObj.getProperty<std::string>("e"), "testing");
  BOOST_CHECK_EQUAL(resObj.getProperty<int>("f"), 1);
  BOOST_CHECK_EQUAL(resObj.getProperty<bool>("g"), false);
}

BOOST_AUTO_TEST_SUITE_END()