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

#include "../core/jshandler.h"

using namespace dss;
using namespace std;

#include <iostream>

BOOST_AUTO_TEST_SUITE(Scripting)

BOOST_AUTO_TEST_CASE(testSimpleScripts) {
  ScriptEnvironment env;
  env.initialize();
  ScriptContext* ctx = env.getContext();
  ctx->loadFromMemory("x = 10; print(x); x = x * x;");
  double result = ctx->evaluate<double>();
  BOOST_CHECK_EQUAL(result, 100.0);
  delete ctx;

  ctx = env.getContext();
  ctx->loadFromFile("data/test.js");
  result = ctx->evaluate<double>();
  BOOST_CHECK_EQUAL(result, 100.0);
  delete ctx;

  ctx = env.getContext();
  ctx->loadFromMemory("x = 'bla'; x = x + 'bla';");
  string sres = ctx->evaluate<string>();
  BOOST_CHECK_EQUAL(sres, string("blabla"));
  delete ctx;
} // testSimpleScripts

BOOST_AUTO_TEST_CASE(testMultipleIterations) {
  ScriptEnvironment env;
  env.initialize();
  for(int i = 0; i < 100; i++) {
    ScriptContext* ctx = env.getContext();
    ctx->loadFromFile("data/test3.js");
    ctx->evaluate<void>();
    cout << ".";
    delete ctx;
  }
  cout << endl;
} // testMultipleIterations

BOOST_AUTO_TEST_CASE(testExceptionHandling) {
  ScriptEnvironment* env = new ScriptEnvironment();
  env->initialize();
  ScriptContext* ctx = env->getContext();
  ctx->loadFromMemory("x = {}; x.what = 'bla'; x.toString = function() { return 'bla'; }; throw x; x = 10;");
  try {
    ctx->evaluate<double>();
    BOOST_CHECK(false);
  } catch(ScriptRuntimeException& _ex) {
    BOOST_CHECK_EQUAL(_ex.getExceptionMessage(), string("bla"));
  }
} // testExceptionHandling

BOOST_AUTO_TEST_SUITE_END()
