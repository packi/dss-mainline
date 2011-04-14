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

#include "core/scripting/scriptobject.h"
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
    if(m_Socket != NULL) {
      m_Socket->close();
    }
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

  boost::shared_ptr<ScriptContext> ctx(env->getContext());
  ctx->evaluate<void>("TcpSocket.sendTo('127.0.0.1', 1234, 'hello');");
  sleepMS(250);
  ctx->stop();
  while(ctx->hasAttachedObjects()) {
    sleepMS(1);
  }
  BOOST_CHECK_EQUAL(listener.m_DataReceived, "hello");
} // testTcpSocketSendTo

BOOST_AUTO_TEST_CASE(testTcpSocketSendToUnconnected) {
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new SocketScriptContextExtension();
  env->addExtension(ext);
  
  boost::shared_ptr<ScriptContext> ctx(env->getContext());
  ctx->evaluate<void>("TcpSocket.sendTo('localhost', 1234, 'h');");
  sleepMS(250);
  ctx->stop();
  while(ctx->hasAttachedObjects()) {
    sleepMS(1);
  }
} // testTcpSocketSendToUnconnected

BOOST_AUTO_TEST_CASE(testTcpSocketSendToNonResolvable) {
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new SocketScriptContextExtension();
  env->addExtension(ext);
  
  boost::shared_ptr<ScriptContext> ctx(env->getContext());
  ctx->evaluate<void>("TcpSocket.sendTo('hurz.dev.digitalstrom.org', 1234, 'h');");
  sleepMS(250);
  ctx->stop();
  while(ctx->hasAttachedObjects()) {
    sleepMS(1);
  }
} // testTcpSocketSendToNonResolvable

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

  boost::shared_ptr<ScriptContext> ctx(env->getContext());
  for(int iRun = 0; iRun < kRuns; iRun++) {
    Logger::getInstance()->log("Before run");
    ctx->evaluate<void>("TcpSocket.sendTo('127.0.0.1', 1234, 'hello');");
    sleepMS(250);
    BOOST_CHECK_EQUAL(listener.m_DataReceived, "hello");
    listener.m_DataReceived.clear();
    Logger::getInstance()->log("After run");
  }
  ctx->stop();
  while(ctx->hasAttachedObjects()) {
    sleepMS(1);
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

  boost::shared_ptr<ScriptContext> ctx(env->getContext());
  {
    ScriptLock lock(ctx);
    JSContextThread req(ctx);
    ctx->getRootObject().setProperty<int>("callCount", 0);
  }
  int callCount = 0;

  foreach(std::string testScript, testScripts) {
    callCount++;
    ctx->evaluate<void>(testScript);
    sleepMS(250);
    BOOST_CHECK_EQUAL(listener.m_DataReceived, "hello");
    ScriptLock lock(ctx);
    JSContextThread req(ctx);
    int newCount = ctx->getRootObject().getProperty<int>("callCount");
    BOOST_CHECK_EQUAL(newCount, callCount);
    callCount = newCount;
    listener.m_DataReceived.clear();
  }
  ctx->stop();
  while(ctx->hasAttachedObjects()) {
    sleepMS(1);
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

  boost::shared_ptr<ScriptContext> ctx(env->getContext());
  ctx->evaluate<void>("socket = new TcpSocket();\n"
                      "socket.connect('localhost', 1234,\n"
                      "  function(success) { result = success; }\n"
                      ");");
  sleepMS(250);

  ScriptLock lock(ctx);
  JSContextThread req(ctx);
  BOOST_CHECK_EQUAL(ctx->getRootObject().getProperty<bool>("result"), true);
  BOOST_CHECK_EQUAL(listener.getConnectionCount(), 1);
  ctx->stop();
  while(ctx->hasAttachedObjects()) {
    sleepMS(1);
  }
}

BOOST_AUTO_TEST_CASE(testSocketConnectFailure) {
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new SocketScriptContextExtension();
  env->addExtension(ext);

  boost::shared_ptr<ScriptContext> ctx(env->getContext());
  ctx->evaluate<void>("socket = new TcpSocket();\n"
                      "socket.connect('localhost', 1234,\n"
                      "  function(success) { result = success; }\n"
                      ");");
  sleepMS(250);
  ScriptLock lock(ctx);
  JSContextThread req(ctx);
  BOOST_CHECK_EQUAL(ctx->getRootObject().getProperty<bool>("result"), false);
  ctx->stop();
  while(ctx->hasAttachedObjects()) {
    sleepMS(1);
  }
}

BOOST_AUTO_TEST_CASE(testSocketConnectResolveFailure) {
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new SocketScriptContextExtension();
  env->addExtension(ext);

  boost::shared_ptr<ScriptContext> ctx(env->getContext());
  ctx->evaluate<void>("socket = new TcpSocket();\n"
                      "socket.connect('hurz.dev.digitalstrom.org', 1234,\n"
                      "  function(success) { result = success; }\n"
                      ");");
  sleepMS(250);
  ScriptLock lock(ctx);
  JSContextThread req(ctx);
  BOOST_CHECK_EQUAL(ctx->getRootObject().getProperty<bool>("result"), false);
  ctx->stop();
  while(ctx->hasAttachedObjects()) {
    sleepMS(1);
  }
}

BOOST_AUTO_TEST_CASE(testSocketSend) {
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new SocketScriptContextExtension();
  env->addExtension(ext);

  TestListener listener(1234);
  listener.run();
  sleepMS(50);
  boost::shared_ptr<ScriptContext> ctx(env->getContext());
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
  ScriptLock lock(ctx);
  JSContextThread req(ctx);
  BOOST_CHECK_EQUAL(listener.m_DataReceived, "hello");
  BOOST_CHECK_EQUAL(ctx->getRootObject().getProperty<int>("bytesSent"), 5);
  ctx->stop();
  while(ctx->hasAttachedObjects()) {
    sleepMS(1);
  }
} // testSocketSend

BOOST_AUTO_TEST_CASE(testSocketClose) {
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new SocketScriptContextExtension();
  env->addExtension(ext);

  TestListener listener(1234);
  listener.run();
  sleepMS(50);

  boost::shared_ptr<ScriptContext> ctx(env->getContext());
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
  ctx->stop();
  while(ctx->hasAttachedObjects()) {
    sleepMS(1);
  }
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

  boost::shared_ptr<ScriptContext> ctx(env->getContext());
  ctx->evaluate<void>(script);
  sleepMS(500);
  ScriptLock lock(ctx);
  JSContextThread req(ctx);
  BOOST_CHECK_EQUAL(ctx->getRootObject().getProperty<std::string>("result"), "hello world");
  ctx->stop();
  while(ctx->hasAttachedObjects()) {
    sleepMS(1);
  }
} // testSocketReceive


void runScript(const std::string& _script, const std::string& _input, const std::string& _expectedResult) {
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new SocketScriptContextExtension();
  env->addExtension(ext);

  boost::shared_ptr<ScriptContext> ctx(env->getContext());
  {
    ScriptLock lock(ctx);
    JSContextThread req(ctx);
    ctx->getRootObject().setProperty<std::string>("input", _input);
  }

  ctx->evaluate<void>(_script);
  sleepMS(500);
  ScriptLock lock(ctx);
  JSContextThread req(ctx);
  std::string result = ctx->getRootObject().getProperty<std::string>("result");
  BOOST_CHECK_EQUAL(result, _expectedResult);
  ctx->stop();
  while(ctx->hasAttachedObjects()) {
    sleepMS(1);
  }
}

const  std::string genericSenderScript =
    "var connected = function(success) {\n"
    "  if(success === true) {\n"
    "    socket.send(input,\n"
    "      function() {\n"
    "        result = input;\n"
    "        socket.close();\n"
    "      }\n"
    "    );\n"
    "  }\n"
    "}\n"
    "socket = new TcpSocket();\n"
    "socket.connect('localhost', 1234, connected);\n";



BOOST_AUTO_TEST_CASE(testSocketServer) {
  std::string serverScript =
    "var handleIncomingSocket = function(incomingSocket) {\n"
    "  incomingSocket.receive(11,\n"
    "    function(line) {\n"
    "      result = line;"
    "    }\n"
    "  );\n"
    "};\n"
    "\n"
    "socket = new TcpSocket();\n"
    "socket.bind(\n"
    "  1234,\n"
    "  function(success) {\n"
    "    if(success === true) {\n"
    "      socket.accept(handleIncomingSocket);\n"
    "    }\n"
    "  }\n"
    ");\n";
  boost::thread serverThread = boost::thread(boost::bind(&runScript, serverScript, "hello world", "hello world"));
  sleepMS(250);
  boost::thread clientThread = boost::thread(boost::bind(&runScript, genericSenderScript, "hello world", "hello world"));
  clientThread.join();
  serverThread.join();
} // testSocketServer

const  std::string lineReaderServerScript =
    "var count = parseInt(input);\n"
    "var result = ''\n"
    "var handleIncomingSocket = function(incomingSocket) {\n"
    "  function readLine() {\n"
    "    incomingSocket.receiveLine(100,\n"
    "      function(line) {\n"
    "        result += line;\n"
    "        count--;\n"
    "        if(count > 0) {\n"
    "          readLine();\n"
    "        }\n"
    "      }\n"
    "    );\n"
    "  }\n"
    "  readLine();\n"
    "};\n"
    "\n"
    "socket = new TcpSocket();\n"
    "socket.bind(\n"
    "  1234,\n"
    "  function(success) {\n"
    "    if(success === true) {\n"
    "      socket.accept(handleIncomingSocket);\n"
    "    }\n"
    "  }\n"
    ");\n";


BOOST_AUTO_TEST_CASE(testSocketReadLine) {
  boost::thread serverThread = boost::thread(boost::bind(&runScript, lineReaderServerScript, "1", "hello world"));
  sleepMS(250);
  boost::thread clientThread = boost::thread(boost::bind(&runScript, genericSenderScript, "hello world\r\n", "hello world\r\n"));
  clientThread.join();
  serverThread.join();
}

BOOST_AUTO_TEST_CASE(testSocketReadLineOnlyOnce) {
  boost::thread serverThread = boost::thread(boost::bind(&runScript, lineReaderServerScript, "1", "hello world"));
  sleepMS(250);
  boost::thread clientThread = boost::thread(boost::bind(&runScript, genericSenderScript, "hello world\r\ntl;dr\r\n", "hello world\r\ntl;dr\r\n"));
  clientThread.join();
  serverThread.join();
}

BOOST_AUTO_TEST_CASE(testSocketReadLineTwice) {
  boost::thread serverThread = boost::thread(boost::bind(&runScript, lineReaderServerScript, "2", "hello worldtl;dr"));
  sleepMS(250);
  boost::thread clientThread = boost::thread(boost::bind(&runScript, genericSenderScript, "hello world\r\ntl;dr\r\n", "hello world\r\ntl;dr\r\n"));
  clientThread.join();
  serverThread.join();
}

BOOST_AUTO_TEST_SUITE_END()
