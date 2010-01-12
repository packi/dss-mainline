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

#include "jssocket.h"

#include "core/thread.h"

#include <boost/noncopyable.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>

using boost::asio::ip::tcp;

namespace dss {


  //================================================== BoostIORunner

  class BoostIORunner : private boost::noncopyable,
                        public Thread {
  public:
    virtual void execute() {
      while(!m_Terminated) {
        m_IOService.run();
      }
    }

    boost::asio::io_service& getIOService() {
      return m_IOService;
    }

    static BoostIORunner& getInstance() {
      if(m_pInstance == NULL) {
        m_pInstance = new BoostIORunner();
      }
      return *m_pInstance;
    }
  private:
    static BoostIORunner* m_pInstance;
    boost::asio::io_service m_IOService;
    BoostIORunner() : Thread("BoostIORunner") {}
  };

  BoostIORunner* BoostIORunner::m_pInstance = NULL;

  //================================================== SocketHelper

  class SocketHelper {
  public:
    SocketHelper(SocketScriptContextExtension& _extension)
    : m_Extension(_extension)
    { }
  protected:
    SocketScriptContextExtension& m_Extension;
  }; // SocketHelper

  class SocketHelperSendOneShot : public SocketHelper,
                                  public boost::enable_shared_from_this<SocketHelperSendOneShot> {
  public:
    SocketHelperSendOneShot(SocketScriptContextExtension& _extension)
    : SocketHelper(_extension),
      m_Socket(BoostIORunner::getInstance().getIOService()),
      m_IOService(BoostIORunner::getInstance().getIOService())
    {
    }
    
    void sendTo(const std::string& _host, int _port, const std::string& _data) {
      m_Data = _data;
      tcp::resolver resolver(m_IOService);
      tcp::resolver::query query(_host, intToString(_port));
      tcp::resolver::iterator iterator = resolver.resolve(query);

      tcp::endpoint endpoint = *iterator;
      m_Socket.async_connect(endpoint,
          boost::bind(&SocketHelperSendOneShot::handle_connect, this,
            boost::asio::placeholders::error, ++iterator));
    }

    void write()
    {
      boost::asio::async_write(m_Socket,
          boost::asio::buffer(m_Data.c_str(),
            m_Data.size()),
          boost::bind(&SocketHelperSendOneShot::handle_write, this,
            boost::asio::placeholders::error));
    }

  private:
    void handle_connect(const boost::system::error_code& error,
        tcp::resolver::iterator endpoint_iterator)
    {
      if (!error) {
        m_IOService.post(boost::bind(&SocketHelperSendOneShot::write, this));
      } else if (endpoint_iterator != tcp::resolver::iterator()) {
        m_Socket.close();
        tcp::endpoint endpoint = *endpoint_iterator;
        m_Socket.async_connect(endpoint,
            boost::bind(&SocketHelperSendOneShot::handle_connect, this,
              boost::asio::placeholders::error, ++endpoint_iterator));
      }
    }

    void handle_write(const boost::system::error_code& error) {
      if(!error) {
        // call js callback(true)
      } else {
        // call js callback(false)
      }
      do_close();
      m_Extension.removeSocketHelper(shared_from_this());
    }

    void do_close() {
      m_Socket.close();
    }
  private:
    tcp::socket m_Socket;
    boost::asio::io_service& m_IOService;
    std::string m_Data;
  }; // SocketHelperOneShot

  //================================================== SocketScriptContextExtension

  const std::string SocketScriptExtensionName = "propertyextension";

  SocketScriptContextExtension::SocketScriptContextExtension()
  : ScriptExtension(SocketScriptExtensionName)
  { }

  void tcpSocket_finalize(JSContext *cx, JSObject *obj) {
  } // tcpSocket_finalize

  JSBool tcpSocket_construct(JSContext *cx, JSObject *obj, uintN argc,
                             jsval *argv, jsval *rval) {
      return JS_TRUE;
  } // tcpSocket_construct

  static JSClass tcpSocket_class = {
    "TcpSocket", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,  JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStandardClasses,
    JS_ResolveStub,
    JS_ConvertStub,  tcpSocket_finalize, JSCLASS_NO_OPTIONAL_MEMBERS
  }; // tcpSocket_class

  JSFunctionSpec tcpSocket_methods[] = {
  //  {"send", tcpSocket_send, 2, 0, 0},
  //  {"connect", tcpSocket_connect, 3, 0, 0},
  //  {"receive", tcpSocket_receive, 2, 0, 0},
  //  {"bind", tcpSocket_bind, 2, 0, 0},
  //  {"accept", tcpSocket_send, 1, 0, 0},
  //  {"close", tcpSocket_close, 0, 0, 0},
    {NULL, NULL, 0, 0, 0},
  }; // tcpSockets_methods

  JSBool tcpSocket_sendTo(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    SocketScriptContextExtension* ext =
       dynamic_cast<SocketScriptContextExtension*>(ctx->getEnvironment().getExtension(SocketScriptExtensionName));

    if(argc >= 3) {
      try {
        std::string host = ctx->convertTo<std::string>(argv[0]);
        int port = ctx->convertTo<int>(argv[1]);
        std::string data = ctx->convertTo<std::string>(argv[2]);

        if(!BoostIORunner::getInstance().isRunning()) {
          BoostIORunner::getInstance().run();
        }
	
	boost::shared_ptr<SocketHelperSendOneShot> helper(new SocketHelperSendOneShot(*ext));
	ext->addSocketHelper(helper);

	helper->sendTo(host, port, data);

        *rval = JSVAL_TRUE;
        return JS_TRUE;
      } catch(const ScriptException& e) {
      }
    }
    return JS_FALSE;
  } // tcpSocket_sendTo

  JSFunctionSpec tcpSocket_static_methods[] = {
    {"sendTo", tcpSocket_sendTo, 4, 0, 0},
    {NULL, NULL, 0, 0, 0},
  }; // tcpSocket_static_methods

  JSBool tcpSocket_JSGet(JSContext *cx, JSObject *obj, jsval id, jsval *rval) {
    int opt = JSVAL_TO_INT(id);
    switch(opt) {
      case 0:
        *rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, "TcpSocket"));
        return JS_TRUE;
    }
    return JS_FALSE;
  } // tcpSocket_JSGet


  static JSPropertySpec tcpSocket_properties[] = {
    {"className", 0, 0, tcpSocket_JSGet},
    {NULL}
  }; // tcpSocket_properties

  void SocketScriptContextExtension::extendContext(ScriptContext& _context) {
    JS_InitClass(_context.getJSContext(), _context.getRootObject().getJSObject(), NULL, &tcpSocket_class,
                 tcpSocket_construct, 0, tcpSocket_properties, tcpSocket_methods, NULL, tcpSocket_static_methods);
  } // extendContext

  void SocketScriptContextExtension::removeSocketHelper(boost::shared_ptr<SocketHelper> _helper) {
    m_SocketHelperMutex.lock();
    SocketHelperVector::iterator it = std::find(m_SocketHelper.begin(), m_SocketHelper.end(), _helper);
    if(it != m_SocketHelper.end()) {
      m_SocketHelper.erase(it);
    }
    m_SocketHelperMutex.unlock();
  } // removeSocketHelper

  void SocketScriptContextExtension::addSocketHelper(boost::shared_ptr<SocketHelper> _helper) {
    m_SocketHelperMutex.lock();
    m_SocketHelper.push_back(_helper);
    m_SocketHelperMutex.unlock();
  } // addSocketHelper


} // namespace dss
