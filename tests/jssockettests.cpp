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
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>

#include "core/scripting/jssocket.h"
#include "core/foreach.h"

using namespace std;
using namespace dss;
using boost::asio::ip::tcp;

BOOST_AUTO_TEST_SUITE(SocketJS)

class TestListener {
public:
  TestListener(int _port)
  : m_IOService(),
    m_Endpoint(tcp::v4(), _port),
    m_Acceptor(m_IOService),
    m_RunsLeft(0),
    m_QuitAfterConnect(false),
    m_ConnectionCount(0)
  {
    m_Acceptor.open(m_Endpoint.protocol());
    m_Acceptor.set_option(tcp::acceptor::reuse_address(true));
    m_Acceptor.bind(m_Endpoint);
    m_Acceptor.listen();
    boost::shared_ptr<tcp::socket> sock(new tcp::socket(m_IOService));
    m_Acceptor.async_accept(*sock, boost::bind(&TestListener::handleConnection, this, sock));
  }

  void run() {
    m_IOServiceThread = boost::thread(boost::bind(&boost::asio::io_service::run, &m_IOService));
  }

  virtual ~TestListener() {
    m_Acceptor.close();
    m_IOServiceThread.join();
    Logger::getInstance()->log("~TestListener");
  }

  void setRuns(const int _value) {
    m_RunsLeft = _value;
  }

  void setQuitAfterConnect(bool _value) {
    m_QuitAfterConnect = _value;
  }

  int getConnectionCount() const {
    return m_ConnectionCount;
  }

  std::string m_DataReceived;
private:

  void handleConnection(boost::shared_ptr<tcp::socket> _sock) {
    if(!m_QuitAfterConnect) {
      char data[100];
      boost::system::error_code error;
      size_t length = _sock->read_some(boost::asio::buffer(data), error);
      if(error == boost::asio::error::eof) {
        return;
      } else if(!error) {
        m_DataReceived.append(data, length);
      }
    }
    m_ConnectionCount = 1;
    _sock->close();
    if(m_RunsLeft > 0) {
      m_RunsLeft--;
      boost::shared_ptr<tcp::socket> sock(new tcp::socket(m_IOService));
      m_Acceptor.async_accept(*sock, boost::bind(&TestListener::handleConnection, this, sock));
    }
  }

  boost::asio::io_service m_IOService;
  tcp::endpoint m_Endpoint;
  tcp::acceptor m_Acceptor;
  int m_RunsLeft;
  boost::thread m_IOServiceThread;
  bool m_QuitAfterConnect;
  int m_ConnectionCount;
};

BOOST_AUTO_TEST_CASE(testTcpSocketSendTo) {
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new SocketScriptContextExtension();
  env->addExtension(ext);

  TestListener listener(1234);
  listener.run();
  sleepMS(50);

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  ctx->evaluate<void>("TcpSocket.sendTo('127.0.0.1', 1234, 'hello');");
  sleepMS(250);
  BOOST_CHECK_EQUAL(listener.m_DataReceived, "hello");
} // testBasics

BOOST_AUTO_TEST_CASE(testTcpSocketSendToRepeatability) {
  const int kRuns = 3;
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new SocketScriptContextExtension();
  env->addExtension(ext);

  TestListener listener(1234);
  listener.setRuns(kRuns);
  listener.run();
  sleepMS(50);

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  for(int iRun = 0; iRun < kRuns; iRun++) {
    ctx->evaluate<void>("TcpSocket.sendTo('127.0.0.1', 1234, 'hello');");
    sleepMS(250);
    BOOST_CHECK_EQUAL(listener.m_DataReceived, "hello");
    listener.m_DataReceived.clear();
  }
} // testRepeatbility

BOOST_AUTO_TEST_CASE(testSendSocketCallback) {
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new SocketScriptContextExtension();
  env->addExtension(ext);

  std::vector<std::string> testScripts;
  testScripts.push_back("TcpSocket.sendTo('127.0.0.1', 1234, 'hello', function(success) { callCount++; });");
  testScripts.push_back("TcpSocket.sendTo('127.0.0.1', 1234, 'hello', function() { callCount++; });");
  testScripts.push_back("TcpSocket.sendTo('127.0.0.1', 1234, 'hello', function(success, otherstuff) { callCount++; });");

  TestListener listener(1234);
  listener.setRuns(testScripts.size());
  listener.run();
  sleepMS(50);

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  ctx->getRootObject().setProperty<int>("callCount", 0);
  int callCount = 0;

  foreach(std::string testScript, testScripts) {
    callCount++;
    ctx->evaluate<void>(testScript);
    sleepMS(250);
    BOOST_CHECK_EQUAL(listener.m_DataReceived, "hello");
    int newCount = ctx->getRootObject().getProperty<int>("callCount");
    BOOST_CHECK_EQUAL(newCount, callCount);
    callCount = newCount;
    listener.m_DataReceived.clear();
  }
} // testSendSocketCallback

BOOST_AUTO_TEST_CASE(testSocketConnect) {
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new SocketScriptContextExtension();
  env->addExtension(ext);

  TestListener listener(1234);
  listener.setQuitAfterConnect(true);
  listener.run();
  sleepMS(50);

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  ctx->evaluate<void>("socket = new TcpSocket();\n"
                      "socket.connect('localhost', 1234);");
  sleepMS(250);
  BOOST_CHECK_EQUAL(listener.getConnectionCount(), 1);
}

BOOST_AUTO_TEST_CASE(testSocketConnectFailure) {
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new SocketScriptContextExtension();
  env->addExtension(ext);

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  ctx->evaluate<void>("socket = new TcpSocket();\n"
  "socket.connect('localhost', 1234);");
  sleepMS(250);
}

BOOST_AUTO_TEST_CASE(testSocketSend) {
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new SocketScriptContextExtension();
  env->addExtension(ext);

  TestListener listener(1234);
  listener.run();
  sleepMS(50);

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  ctx->evaluate<void>("socket = new TcpSocket();\n"
                      "socket.connect('localhost', 1234,\n"
                      "      function(success) {\n"
                      "        if(success) {\n"
                      "          socket.send('hello');\n"
                      "        }\n"
                      "      }\n"
                      ");");
  sleepMS(250);
  BOOST_CHECK_EQUAL(listener.m_DataReceived, "hello");
} // testSocketSend

BOOST_AUTO_TEST_CASE(testSocketClose) {
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new SocketScriptContextExtension();
  env->addExtension(ext);

  TestListener listener(1234);
  listener.run();
  sleepMS(50);

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  ctx->evaluate<void>("socket = new TcpSocket();\n"
                      "socket.connect('localhost', 1234,\n"
                      "      function(success) {\n"
                      "        if(success) {\n"
                      "          socket.send('hello');\n"
                      "          socket.close();\n"
                      "        }\n"
                      "      }\n"
                      ");");
  sleepMS(250);
  BOOST_CHECK_EQUAL(listener.m_DataReceived, "hello");
} // testSocketClose

BOOST_AUTO_TEST_SUITE_END()
