/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland

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
#include <boost/make_shared.hpp>

#include "src/scripting/jshandler.h"
#include "src/scripting/scriptobject.h"

using namespace dss;
using namespace std;

#include <iostream>

BOOST_AUTO_TEST_SUITE(Scripting)

BOOST_AUTO_TEST_CASE(testSimpleScripts) {
  ScriptEnvironment env;
  env.initialize();
  boost::scoped_ptr<ScriptContext> ctx(env.getContext());
  double result = ctx->evaluate<double>("x = 10; print(x); x = x * x;");
  BOOST_CHECK_EQUAL(result, 100.0);
} // testSimpleScripts

BOOST_AUTO_TEST_CASE(testSimpleScripts2) {
  ScriptEnvironment env;
  env.initialize();
  boost::scoped_ptr<ScriptContext> ctx(env.getContext());
  std::string fileName = getTempDir() + "/test.js";
  std::ofstream ofs(fileName.c_str());
  ofs << "function newTestObj() {\n"
         "  var result = {};\n"
         "  result.x = 10;\n"
         "  return result;\n"
         "}\n"
         "\n"
         "var obj = newTestObj();\n"
         "obj.x = obj.x * obj.x;\n"
         "print(obj.x);\n"
         "obj.x;\n";
  ofs.close();
  double result = ctx->evaluateScript<double>(fileName);
  BOOST_CHECK_EQUAL(result, 100.0);
  boost::filesystem::remove_all(fileName);
} // testSimpleScripts2

BOOST_AUTO_TEST_CASE(testSimpleScripts3) {
  ScriptEnvironment env;
  env.initialize();
  boost::scoped_ptr<ScriptContext> ctx(env.getContext());

  string sres = ctx->evaluate<string>("x = 'bla'; x = x + 'bla';");
  BOOST_CHECK_EQUAL(sres, string("blabla"));
} // testSimpleScripts3

BOOST_AUTO_TEST_CASE(testMultipleIterations) {
  ScriptEnvironment env;
  env.initialize();
  std::string fileName = getTempDir() + "/test3.js";
  std::ofstream ofs(fileName.c_str());
  ofs << "var x = 0;\nx = x + 1;";
  ofs.close();
  for(int i = 0; i < 100; i++) {
    boost::scoped_ptr<ScriptContext> ctx(env.getContext());
    ctx->evaluateScript<void>(fileName);
    cout << ".";
  }
  cout << endl;
  boost::filesystem::remove_all(fileName);
} // testMultipleIterations

BOOST_AUTO_TEST_CASE(testExceptionHandling) {
  boost::shared_ptr<ScriptEnvironment> env = boost::make_shared<ScriptEnvironment>();
  env->initialize();
  boost::shared_ptr<ScriptContext> ctx(env->getContext());

  // verify that the local "throw x" exception is handled withing the engine itself
  ctx->evaluate<double>("x = {}; x.what = 'bla'; x.toString = function() { return 'bla'; }; x = 5; throw x; x = 10;");

  JSContextThread req(ctx);
  BOOST_CHECK_EQUAL(ctx->getRootObject().getProperty<int>("x"), 5);
} // testExceptionHandling

BOOST_AUTO_TEST_SUITE_END()
