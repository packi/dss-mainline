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

#include <boost/scoped_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>

#include "core/scripting/scriptobject.h"
#include "core/scripting/ds485scriptextension.h"
#include "core/foreach.h"
#include "core/DS485Interface.h"
#include "core/ds485/businterfacehandler.h"

using namespace std;
using namespace dss;

BOOST_AUTO_TEST_SUITE(DS485JS)

class FakeFrameSenderCounter : public FrameSenderInterface {
public:
  FakeFrameSenderCounter()
  : m_Counter(0)
  {}

  virtual void sendFrame(DS485CommandFrame& _frame) {
    m_Counter++;
  }

  int getCounter() const { return m_Counter; }
private:
  int m_Counter;
};

BOOST_AUTO_TEST_CASE(testSendFrameWorksWithoutCallback) {
  FakeFrameSenderCounter frameSender;
  FrameBucketHolder bucketHolder;
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new DS485ScriptExtension(frameSender, bucketHolder);
  env->addExtension(ext);

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  ctx->evaluate<void>("var frame = new DS485Frame();\n"
                      "frame.payload.push(0xaabb);\n"
                      "frame.payload.push(0xccdd);\n"
                      "DS485.sendFrame(frame);");
  BOOST_CHECK_EQUAL(frameSender.getCounter(), 1);
}

BOOST_AUTO_TEST_CASE(testSendFrameWorksWithCallback) {
  FakeFrameSenderCounter frameSender;
  FrameBucketHolder bucketHolder;
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new DS485ScriptExtension(frameSender, bucketHolder);
  env->addExtension(ext);

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  ctx->evaluate<void>("var frame = new DS485Frame();\n"
                      "frame.payload.push(0xaabb);\n"
                      "frame.payload.push(0xccdd);\n"
                      "DS485.sendFrame(frame, function() { return false; }, 10);");
  while(ctx->hasAttachedObjects()) {
    sleepMS(1);
  }
  BOOST_CHECK_EQUAL(frameSender.getCounter(), 1);
}

BOOST_AUTO_TEST_CASE(testSendFrameWorksWithNoIncomingFrame) {
  FakeFrameSenderCounter frameSender;
  FrameBucketHolder bucketHolder;
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new DS485ScriptExtension(frameSender, bucketHolder);
  env->addExtension(ext);

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  ctx->getRootObject().setProperty<bool>("result", false);
  ctx->evaluate<void>("var frame = new DS485Frame();\n"
                      "frame.functionID = 10;\n"
                      "frame.destination = 20;\n"
                      "frame.payload.push(0xaabb);\n"
                      "frame.payload.push(0xccdd);\n"
                      "DS485.sendFrame(frame, function(f) { if(f == null) { result = true; } return false; }, 5);");
  while(ctx->hasAttachedObjects()) {
    sleepMS(1);
  }
  BOOST_CHECK_EQUAL(frameSender.getCounter(), 1);
  BOOST_CHECK_EQUAL(ctx->getRootObject().getProperty<bool>("result"), true);
}

BOOST_AUTO_TEST_CASE(testSendFrameWorksWithIncomingFrame) {
  FakeFrameSenderCounter frameSender;
  FrameBucketHolder bucketHolder;
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new DS485ScriptExtension(frameSender, bucketHolder);
  env->addExtension(ext);

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  ctx->getRootObject().setProperty<bool>("result", false);
  ctx->evaluate<void>("var frame = new DS485Frame();\n"
                      "frame.functionID = 10;\n"
                      "frame.destination = 20;\n"
                      "frame.payload.push(0xaabb);\n"
                      "frame.payload.push(0xccdd);\n"
                      "DS485.sendFrame(frame, function(f) { result = f.functionID == 10; return false; }, 40);");
  sleepMS(10);
  boost::shared_ptr<DS485CommandFrame> frame(new DS485CommandFrame());
  frame->getHeader().setSource(20);
  frame->getPayload().add<uint8_t>(10);
  bucketHolder.distributeFrame(frame, 10);
  while(ctx->hasAttachedObjects()) {
    sleepMS(1);
  }
  BOOST_CHECK_EQUAL(frameSender.getCounter(), 1);
  BOOST_CHECK_EQUAL(ctx->getRootObject().getProperty<bool>("result"), true);
}

BOOST_AUTO_TEST_CASE(testSendFrameWorksRepeatedlyWithIncomingFrame) {
  FakeFrameSenderCounter frameSender;
  FrameBucketHolder bucketHolder;
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new DS485ScriptExtension(frameSender, bucketHolder);
  env->addExtension(ext);

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  ctx->getRootObject().setProperty<int>("result", 0);
  ctx->evaluate<void>("var frame = new DS485Frame();\n"
                      "frame.functionID = 10;\n"
                      "frame.destination = 20;\n"
                      "frame.payload.push(0xaabb);\n"
                      "frame.payload.push(0xccdd);\n"
                      "DS485.sendFrame(frame, function(f) { result++; return (result < 2); }, 15);");
  sleepMS(10);
  boost::shared_ptr<DS485CommandFrame> frame(new DS485CommandFrame());
  frame->getHeader().setSource(20);
  frame->getPayload().add<uint8_t>(10);
  bucketHolder.distributeFrame(frame, 10);
  bucketHolder.distributeFrame(frame, 10);
  while(ctx->hasAttachedObjects()) {
    sleepMS(1);
  }
  BOOST_CHECK_EQUAL(frameSender.getCounter(), 1);
  BOOST_CHECK_EQUAL(ctx->getRootObject().getProperty<int>("result"), 2);
}

BOOST_AUTO_TEST_CASE(testSetCallbackWorks) {
  FakeFrameSenderCounter frameSender;
  FrameBucketHolder bucketHolder;
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new DS485ScriptExtension(frameSender, bucketHolder);
  env->addExtension(ext);

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  ctx->getRootObject().setProperty<int>("result", 0);
  ctx->evaluate<void>("DS485.setCallback(function(f) { result++; return (result < 2); }, 20, 15);");
  sleepMS(20);
  boost::shared_ptr<DS485CommandFrame> frame(new DS485CommandFrame());
  frame->getHeader().setSource(20);
  frame->getPayload().add<uint8_t>(10);
  bucketHolder.distributeFrame(frame, 10);
  bucketHolder.distributeFrame(frame, 10);
  while(ctx->hasAttachedObjects()) {
    sleepMS(1);
  }
  BOOST_CHECK_EQUAL(ctx->getRootObject().getProperty<int>("result"), 2);
}

BOOST_AUTO_TEST_SUITE_END()
