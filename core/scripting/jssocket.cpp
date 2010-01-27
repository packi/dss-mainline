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

#include <boost/noncopyable.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/thread.hpp>

using boost::asio::ip::tcp;

namespace dss {


  //================================================== SocketHelper

  class SocketHelper {
  public:
    SocketHelper(SocketScriptContextExtension& _extension)
    : m_Extension(_extension),
      m_IOService(),
      m_CallbackFunction(JSVAL_NULL)
    { }

    ~SocketHelper() {
      m_IOServiceThread.join();
    }

    void setContext(ScriptContext* _value) {
      m_pContext = _value;
    }

    void setCallbackObject(boost::shared_ptr<ScriptObject> _value) {
      m_pCallbackObject = _value;
    }

    void setCallbackFunction(jsval _value) {
      m_CallbackFunction = _value;
    }

    bool hasCallback() const {
      return (m_pCallbackObject != NULL) && (m_CallbackFunction != JSVAL_NULL);
    }

    void callCallbackWithArguments(ScriptFunctionParameterList& _list) {
      if(hasCallback()) {
        // copy callback data so we can clear the originals
        boost::shared_ptr<ScriptObject> pCallbackObjectCopy = m_pCallbackObject;
        jsval callbackFunctionCopy = m_CallbackFunction;
        // clear callback objects before calling the callback, we might overwrite
        // data if we do that afterwards
        m_pCallbackObject.reset();
        m_CallbackFunction = JSVAL_NULL;
        try {
          pCallbackObjectCopy->callFunctionByReference<void>(callbackFunctionCopy, _list);
        } catch(ScriptException& e) {
          Logger::getInstance()->log(
               std::string("SocketHelper::callCallbackWithArguments: Exception caught: ")
               + e.what(), lsError);
        }
      }
    }

    ScriptContext& getContext() const {
      assert(m_pContext != NULL);
      return *m_pContext;
    }

    void startIOThread() {
      m_IOServiceThread = boost::thread(boost::bind(&boost::asio::io_service::run, &m_IOService));
    }

    boost::asio::io_service& getIOService() {
      return m_IOService;
    }

  protected:
    SocketScriptContextExtension& m_Extension;
  private:
    boost::asio::io_service m_IOService;
    boost::thread m_IOServiceThread;
    ScriptContext* m_pContext;
    boost::shared_ptr<ScriptObject> m_pCallbackObject;
    jsval m_CallbackFunction;
  }; // SocketHelper

  class SocketHelperInstance : public SocketHelper,
                               public boost::enable_shared_from_this<SocketHelperInstance> {
  public:
    SocketHelperInstance(SocketScriptContextExtension& _extension)
    : SocketHelper(_extension),
      m_Socket(getIOService())
    { }

    void connectTo(const std::string& _host, int _port) {
      tcp::resolver resolver(getIOService());
      tcp::resolver::query query(_host, intToString(_port));
      tcp::resolver::iterator iterator = resolver.resolve(query);

      tcp::endpoint endpoint = *iterator;
      m_Socket.async_connect(endpoint,
                             boost::bind(&SocketHelperInstance::connectionCallback, this,
                                         boost::asio::placeholders::error, ++iterator));
      startIOThread();
    }

    void send(const std::string& _data) {
      m_Data = _data;
      boost::asio::async_write(m_Socket,
                               boost::asio::buffer(m_Data.c_str(),
                                                   m_Data.size()),
                               boost::bind(&SocketHelperInstance::sendCallback,
                                           this,
                                           boost::asio::placeholders::error,
                                           boost::asio::placeholders::bytes_transferred));
      startIOThread();
    }

    void close() {
      m_Socket.close();
    }

    bool isConnected() {
      return m_Socket.is_open();
    }
  private:
    void connectionCallback(const boost::system::error_code& error,
                        tcp::resolver::iterator endpoint_iterator) {
      if (!error) {
        success();
      } else if (endpoint_iterator != tcp::resolver::iterator()) {
        m_Socket.close();
        tcp::endpoint endpoint = *endpoint_iterator;
        m_Socket.async_connect(endpoint,
                               boost::bind(&SocketHelperInstance::connectionCallback, this,
                                           boost::asio::placeholders::error, ++endpoint_iterator));
      } else {
        Logger::getInstance()->log("SocketHelperInstance::connectionCallback: failed: " + error.message());
        failure();
      }
    } // connectionCallback

    void sendCallback(const boost::system::error_code& error, std::size_t bytesTransfered) {
      if(!error) {
        if(bytesTransfered == m_Data.size()) {
          m_Data.clear();
          callSizeCallback(bytesTransfered);
        }
      } else {
        Logger::getInstance()->log("SocketHelperInstance::sendCallback: error: " + error.message());
        callSizeCallback(-1);
      }
    }

    void success() {
      callCallback(true);
    }

    void failure() {
      callCallback(false);
    }

    void callCallback(bool _result) {
      ScriptFunctionParameterList param(getContext());
      param.add<bool>(_result);
      callCallbackWithArguments(param);
    }

    void callSizeCallback(int _result) {
      ScriptFunctionParameterList param(getContext());
      param.add<int>(_result);
      callCallbackWithArguments(param);
    }
  private:
    tcp::socket m_Socket;
    std::string m_Data;
  }; // SocketHelperInstance

  class SocketHelperSendOneShot : public SocketHelper,
                                  public boost::enable_shared_from_this<SocketHelperSendOneShot> {
  public:
    SocketHelperSendOneShot(SocketScriptContextExtension& _extension)
    : SocketHelper(_extension),
      m_Socket(getIOService())
    {
    }

    void sendTo(const std::string& _host, int _port, const std::string& _data) {
      m_Data = _data;
      tcp::resolver resolver(getIOService());
      tcp::resolver::query query(_host, intToString(_port));
      tcp::resolver::iterator iterator = resolver.resolve(query);

      tcp::endpoint endpoint = *iterator;
      m_Socket.async_connect(endpoint,
          boost::bind(&SocketHelperSendOneShot::handle_connect, this,
            boost::asio::placeholders::error, ++iterator));
      startIOThread();
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
        getIOService().post(boost::bind(&SocketHelperSendOneShot::write, this));
      } else if (endpoint_iterator != tcp::resolver::iterator()) {
        m_Socket.close();
        tcp::endpoint endpoint = *endpoint_iterator;
        m_Socket.async_connect(endpoint,
            boost::bind(&SocketHelperSendOneShot::handle_connect, this,
              boost::asio::placeholders::error, ++endpoint_iterator));
      }
    }

    void handle_write(const boost::system::error_code& error) {
      if(hasCallback()) {
        ScriptFunctionParameterList params(getContext());
        params.add<bool>(!error);
        callCallbackWithArguments(params);
        if(error) {
          Logger::getInstance()->log("SocketHelperSendOneShot::handle_write: " + error.message());
        }
      }
      do_close();
      m_Extension.removeSocketHelper(shared_from_this());
    }

    void do_close() {
      m_Socket.close();
    }
  private:
    tcp::socket m_Socket;
    std::string m_Data;
  }; // SocketHelperOneShot

  //================================================== SocketScriptContextExtension

  const std::string SocketScriptExtensionName = "propertyextension";

  SocketScriptContextExtension::SocketScriptContextExtension()
  : ScriptExtension(SocketScriptExtensionName)
  { }

  void tcpSocket_finalize(JSContext *cx, JSObject *obj) {
    SocketHelperInstance* pInstance = static_cast<SocketHelperInstance*>(JS_GetPrivate(cx, obj));
    JS_SetPrivate(cx, obj, NULL);
    delete pInstance;
  } // tcpSocket_finalize

  JSBool tcpSocket_construct(JSContext *cx, JSObject *obj, uintN argc,
                             jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    SocketScriptContextExtension* ext =
      dynamic_cast<SocketScriptContextExtension*>(ctx->getEnvironment().getExtension(SocketScriptExtensionName));
    SocketHelperInstance* inst = new SocketHelperInstance(*ext);
    JS_SetPrivate(cx, obj, inst);
    return JS_TRUE;
  } // tcpSocket_construct

  static JSClass tcpSocket_class = {
    "TcpSocket", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,  JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStandardClasses,
    JS_ResolveStub,
    JS_ConvertStub,  tcpSocket_finalize, JSCLASS_NO_OPTIONAL_MEMBERS
  }; // tcpSocket_class

  JSBool tcpSocket_connect(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    SocketHelperInstance* pInst = static_cast<SocketHelperInstance*>(JS_GetPrivate(cx, obj));
    assert(pInst != NULL);
    if(argc >= 2) {
      try {
        std::string host = ctx->convertTo<std::string>(argv[0]);
        int port = ctx->convertTo<int>(argv[1]);

        pInst->setContext(ctx);

        // check if we've been given a callback
        if(argc >= 3) {
          boost::shared_ptr<ScriptObject> scriptObj(new ScriptObject(obj, *ctx));
          pInst->setCallbackObject(scriptObj);
          pInst->setCallbackFunction(argv[2]);
        }

        pInst->connectTo(host, port);
        *rval = JSVAL_TRUE;
        return JS_TRUE;
      } catch(const ScriptException& e) {
        Logger::getInstance()->log(std::string("tcpSocket_connect: Caught script exception: ") + e.what());
      }
    }
    return JS_FALSE;
  } // tcpSocket_connect

  JSBool tcpSocket_send(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    SocketHelperInstance* pInst = static_cast<SocketHelperInstance*>(JS_GetPrivate(cx, obj));
    assert(pInst != NULL);
    if(pInst->isConnected()) {
      if(argc >= 1) {
        try {
          std::string data = ctx->convertTo<std::string>(argv[0]);

          pInst->setContext(ctx);

          // check if we've been given a callback
          if(argc >= 2) {
            boost::shared_ptr<ScriptObject> scriptObj(new ScriptObject(obj, *ctx));
            pInst->setCallbackObject(scriptObj);
            pInst->setCallbackFunction(argv[1]);
          }

          pInst->send(data);
          *rval = JSVAL_TRUE;
          return JS_TRUE;
        } catch(const ScriptException& e) {
          Logger::getInstance()->log(std::string("tcpSocket_send: Caught script exception: ") + e.what());
        }
      }
    } else {
      Logger::getInstance()->log("tcpSocket_send: not connected, please call connect first", lsError);
    }
    return JS_FALSE;
  } // tcpSocket_send

  JSBool tcpSocket_close(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    SocketHelperInstance* pInst = static_cast<SocketHelperInstance*>(JS_GetPrivate(cx, obj));
    assert(pInst != NULL);
    pInst->close();
    *rval = JSVAL_TRUE;
    return JS_TRUE;
  } // tcpSocket_connect

  JSFunctionSpec tcpSocket_methods[] = {
    {"send", tcpSocket_send, 2, 0, 0},
    {"connect", tcpSocket_connect, 3, 0, 0},
  //  {"receive", tcpSocket_receive, 2, 0, 0},
  //  {"bind", tcpSocket_bind, 2, 0, 0},
  //  {"accept", tcpSocket_send, 1, 0, 0},
    {"close", tcpSocket_close, 0, 0, 0},
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

        boost::shared_ptr<SocketHelperSendOneShot> helper(new SocketHelperSendOneShot(*ext));
        ext->addSocketHelper(helper);
        helper->setContext(ctx);

        // check if we've been given a callback
        if(argc > 3) {
          boost::shared_ptr<ScriptObject> scriptObj(new ScriptObject(obj, *ctx));
          helper->setCallbackObject(scriptObj);
          helper->setCallbackFunction(argv[3]);
        }

        helper->sendTo(host, port, data);

        *rval = JSVAL_TRUE;
        return JS_TRUE;
      } catch(const ScriptException& e) {
        Logger::getInstance()->log(std::string("tcpSocket_sendTo: Caught script exception: ") + e.what());
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
