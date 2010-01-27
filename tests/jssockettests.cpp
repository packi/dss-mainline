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
    m_ConnectionCount(0),
    m_Echo(false)
  {
    m_Acceptor.open(m_Endpoint.protocol());
    m_Acceptor.set_option(tcp::acceptor::reuse_address(true));
    m_Acceptor.bind(m_Endpoint);
    m_Acceptor.listen();
    m_Socket.reset(new tcp::socket(m_IOService));
    m_Acceptor.async_accept(*m_Socket, boost::bind(&TestListener::handleConnection, this, m_Socket));
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

  void setEcho(bool _value) {
    m_Echo = _value;
  }

  std::string m_DataReceived;
private:

  void handleConnection(boost::shared_ptr<tcp::socket> _sock) {
    if(!m_QuitAfterConnect) {
      boost::system::error_code error;
      size_t length = _sock->read_some(boost::asio::buffer(m_Data, kMaxDataLength), error);
      if(error == boost::asio::error::eof) {
        return;
      } else if(!error) {
        m_DataReceived.append(m_Data, length);
        if(m_Echo) {
          sendEcho(_sock, length);
        }
      }
    }
    m_ConnectionCount++;
    if(!m_Echo) {
      nextConnection();
    }
  }

  void nextConnection() {
    m_Socket->close();
    if(m_RunsLeft > 0) {
      m_RunsLeft--;
      m_Socket.reset(new tcp::socket(m_IOService));
      m_Acceptor.async_accept(*m_Socket, boost::bind(&TestListener::handleConnection, this, m_Socket));
    }
  } // nextConnection

  void sendEcho(boost::shared_ptr<tcp::socket> _socket, int _length) {
    boost::asio::async_write(
      *_socket,
      boost::asio::buffer(m_Data, _length),
      boost::bind(&TestListener::handleWritten, this, _socket, boost::asio::placeholders::error)
    );
  }

  void handleWritten(boost::shared_ptr<tcp::socket> _socket, const boost::system::error_code& _error) {
    nextConnection();
  }

  boost::asio::io_service m_IOService;
  tcp::endpoint m_Endpoint;
  tcp::acceptor m_Acceptor;
  int m_RunsLeft;
  boost::thread m_IOServiceThread;
  bool m_QuitAfterConnect;
  int m_ConnectionCount;
  bool m_Echo;
  enum { kMaxDataLength = 100 };
  char m_Data[kMaxDataLength];
  boost::shared_ptr<tcp::socket> m_Socket;
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
  ctx->getRootObject().setProperty<bool>("result", false);
  ctx->evaluate<void>("socket = new TcpSocket();\n"
                      "socket.connect('localhost', 1234,\n"
                      "  function(success) { result = success; }\n"
                      ");");
  sleepMS(250);
  BOOST_CHECK_EQUAL(ctx->getRootObject().getProperty<bool>("result"), true);
  BOOST_CHECK_EQUAL(listener.getConnectionCount(), 1);
}

BOOST_AUTO_TEST_CASE(testSocketConnectFailure) {
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new SocketScriptContextExtension();
  env->addExtension(ext);

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  ctx->getRootObject().setProperty<bool>("result", true);
  ctx->evaluate<void>("socket = new TcpSocket();\n"
                      "socket.connect('localhost', 1234,\n"
                      "  function(success) { result = success; }\n"
                      ");");
  sleepMS(250);
  BOOST_CHECK_EQUAL(ctx->getRootObject().getProperty<bool>("result"), false);
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
  ctx->getRootObject().setProperty<int>("bytesSent", 0);
  ctx->evaluate<void>("socket = new TcpSocket();\n"
                      "socket.connect('localhost', 1234,\n"
                      "      function(success) {\n"
                      "        if(success) {\n"
                      "          socket.send('hello',\n"
                      "            function(sent) {\n"
                      "              bytesSent = sent;\n"
                      "            }\n"
                      "          );\n"
                      "        }\n"
                      "      }\n"
                      ");");
  sleepMS(250);
  BOOST_CHECK_EQUAL(listener.m_DataReceived, "hello");
  BOOST_CHECK_EQUAL(ctx->getRootObject().getProperty<int>("bytesSent"), 5);
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
                      "socket.connect('127.0.0.1', 1234,\n"
                      "      function(success) {\n"
                      "        if(success) {\n"
                      "          socket.send('hello', \n"
                      "           function(bytesSent) {\n"
                      "             socket.close();\n"
                      "           });\n"
                      "        }\n"
                      "      }\n"
                      ");");
  sleepMS(250);
  BOOST_CHECK_EQUAL(listener.m_DataReceived, "hello");
} // testSocketClose

BOOST_AUTO_TEST_CASE(testSocketReceive) {
  std::string script =
    "socket = new TcpSocket();\n"
    "socket.connect('localhost', 1234,\n"
    "  function(success) {\n"
    "    if(success === true) {\n"
    "      socket.send('hello world',\n"
    "        function(bytesSent) {\n"
    "          socket.receive(11,\n"
    "            function(data) {\n"
    "              print('received: ', data);\n"
    "              result = data;\n"
    "              socket.close();\n"
    "            }\n"
    "          );\n"
    "        }\n"
    "      );\n"
    "    }\n"
    "  }\n"
    ");\n";
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new SocketScriptContextExtension();
  env->addExtension(ext);

  TestListener listener(1234);
  listener.setEcho(true);
  listener.run();
  sleepMS(50);

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  ctx->getRootObject().setProperty<const char*>("result", "");
  ctx->evaluate<void>(script);
  sleepMS(500);
  BOOST_CHECK_EQUAL(ctx->getRootObject().getProperty<std::string>("result"), "hello world");
} // testSocketReceive

BOOST_AUTO_TEST_SUITE_END()
