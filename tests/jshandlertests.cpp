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

  ctx->loadFromMemory("obj.testing");
  BOOST_CHECK_EQUAL(ctx->evaluate<int>(), 1);
  obj.setProperty("testing", 0);
  BOOST_CHECK_EQUAL(ctx->evaluate<int>(), 0);

  obj.setProperty("testing", "test");
  BOOST_CHECK_EQUAL(obj.getProperty<std::string>("testing"), "test");
  BOOST_CHECK_EQUAL(ctx->evaluate<std::string>(), "test");
} // testSimpleObject

BOOST_AUTO_TEST_SUITE_END()
