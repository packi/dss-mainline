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

#include "core/scripting/jssocket.h"

#include <boost/scoped_ptr.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include "core/thread.h"

using namespace std;
using namespace dss;
using boost::asio::ip::tcp;

BOOST_AUTO_TEST_SUITE(SocketJS)

class TestListener : public Thread {
public:
  TestListener(int _port) 
  : m_IOService(),
    m_Endpoint(tcp::v4(), _port),
    m_Acceptor(m_IOService, m_Endpoint)
  {
    boost::shared_ptr<tcp::socket> sock(new tcp::socket(m_IOService));
    m_Acceptor.async_accept(*sock, boost::bind(&TestListener::handleConnection, this, sock));
  }
  
  virtual void execute() {
    while(!m_Terminated) {
      m_IOService.run();
    }
  }
  
  std::string m_DataReceived;  
  boost::asio::io_service m_IOService;
private:
  
  void handleConnection(boost::shared_ptr<tcp::socket> _sock) {
    char data[100];
    boost::system::error_code error;
    _sock->read_some(boost::asio::buffer(data), error);
    if(error == boost::asio::error::eof) {
      return; 
    } else if(!error) {
      m_DataReceived = data;
    }
  }
  
  tcp::endpoint m_Endpoint;
  tcp::acceptor m_Acceptor;
};


BOOST_AUTO_TEST_CASE(testBasics) {
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new SocketScriptContextExtension();
  env->addExtension(ext);
  
  TestListener listener(1234);
  listener.run();

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  ctx->evaluate<void>("TcpSocket.sendTo('127.0.0.1', 1234, 'hello');");
  sleepSeconds(2);
  BOOST_CHECK_EQUAL(listener.m_DataReceived, "hello");
  listener.m_IOService.stop();
  listener.terminate();
  sleepSeconds(2);
}

BOOST_AUTO_TEST_SUITE_END()
