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
#if defined(__CYGWIN__)
#define __USE_W32_SOCKETS 1
#endif

#include "jssocket.h"

#include <boost/noncopyable.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/thread.hpp>

using boost::asio::ip::tcp;

#include "core/security/security.h"
#include "core/security/user.h"
#include "core/scripting/scriptobject.h"

namespace dss {
  void tcpSocket_finalize(JSContext *cx, JSObject *obj);
  JSBool tcpSocket_construct(JSContext *cx, JSObject *obj, uintN argc,
                             jsval *argv, jsval *rval);

  static JSClass tcpSocket_class = {
    "TcpSocket", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,  JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStandardClasses,
    JS_ResolveStub,
    JS_ConvertStub,  tcpSocket_finalize, JSCLASS_NO_OPTIONAL_MEMBERS
  }; // tcpSocket_class


  class SocketHelperBase {
  public:
    virtual void stop() = 0;
  };

  //================================================== SocketAttachedObject

  class SocketAttachedObject : public ScriptContextAttachedObject {
  public:
    SocketAttachedObject(ScriptContext* _pContext, SocketHelperBase* _pBase)
    : ScriptContextAttachedObject(_pContext),
      m_pBase(_pBase)
    { }

    virtual void stop() {
      m_pBase->stop();
    }
  private:
    SocketHelperBase* m_pBase;
  }; // SocketAttachedObject

  //================================================== SocketHelper

  class SocketHelper : public SocketHelperBase {
  public:
    SocketHelper(SocketScriptContextExtension& _extension)
    : m_Extension(_extension),
      m_CallbackFunction(JSVAL_NULL)
    { }

    ~SocketHelper() {
      m_pASIOWork.reset();
      if(m_IOService != NULL) {
        m_IOService->stop();
      }
      m_IOServiceThread.join();
      Logger::getInstance()->log("~SocketHelper()");
    }

    void setContext(ScriptContext* _value) {
      m_pContext = _value;
    }

    void setCallbackObject(boost::shared_ptr<ScriptObject> _value) {
      m_pCallbackObject = _value;
    }

    void setCallbackFunction(jsval _value) {
      m_pFunctionRooter.reset(new ScriptFunctionRooter(m_pContext, m_pCallbackObject->getJSObject(), _value));
      m_CallbackFunction = _value;
    }

    bool hasCallback() const {
      return (m_pCallbackObject != NULL) && (m_CallbackFunction != JSVAL_NULL);
    }

    void releaseCallbackObjects() {
      m_pFunctionRooter.reset();
      m_pCallbackObject.reset();
      m_CallbackFunction = JSVAL_NULL;
    }

    void callCallbackWithArguments(ScriptFunctionParameterList& _list) {
      if(hasCallback()) {
        JSAutoLocalRootScope rootScope(getContext().getJSContext());
        // copy callback data so we can clear the originals
        boost::shared_ptr<ScriptObject> pCallbackObjectCopy = m_pCallbackObject;
        jsval callbackFunctionCopy = m_CallbackFunction;
        boost::shared_ptr<ScriptFunctionRooter> functionRoot = m_pFunctionRooter;
        // clear callback objects before calling the callback, we might overwrite
        // data if we do that afterwards
        m_pFunctionRooter.reset();
        m_pCallbackObject.reset();
        m_CallbackFunction = JSVAL_NULL;
        try {
          Logger::getInstance()->log("Before calling callback");
          pCallbackObjectCopy->callFunctionByReference<void>(callbackFunctionCopy, _list);
          Logger::getInstance()->log("After calling callback");
        } catch(ScriptException& e) {
          Logger::getInstance()->log(
               std::string("SocketHelper::callCallbackWithArguments: Exception caught: ")
               + e.what(), lsError);
        }
      }
      JS_GC(m_pContext->getJSContext());
    }

    ScriptContext& getContext() const {
      assert(m_pContext != NULL);
      return *m_pContext;
    }

    boost::asio::io_service& getIOService() {
      ensureIOServiceAvailable();
      return *m_IOService;
    }

    boost::shared_ptr<boost::asio::io_service> getIOServicePtr() {
      return m_IOService;
    }

    void setIOService(boost::shared_ptr<boost::asio::io_service> _value) {
      m_IOService = _value;
    }

  protected:
    void startBlockingCall() {
      m_pAttachedObject.reset(new SocketAttachedObject(&getContext(), this));
    }

    boost::shared_ptr<ScriptContextAttachedObject> endBlockingCall() {
      boost::shared_ptr<ScriptContextAttachedObject> result = m_pAttachedObject;
      m_pAttachedObject.reset();
      return result;
    }
  protected:
    SocketScriptContextExtension& m_Extension;
  private:
    void runIOService(User* _pUser) {
      assert(m_pContext != NULL);
      if(_pUser != NULL) {
        Security* pSecurity = m_pContext->getEnvironment().getSecurity();
        if(pSecurity != NULL) {
          pSecurity->signIn(_pUser);
        }
        delete _pUser;
      }
      m_IOService->run();
    }
    void ensureIOServiceAvailable() {
      if(m_IOService == NULL) {
        assert(m_pContext != NULL);
        m_IOService.reset(new boost::asio::io_service());
        m_pASIOWork.reset(new boost::asio::io_service::work(*m_IOService));
        User* pUser = NULL;
        if(Security::getCurrentlyLoggedInUser() != NULL) {
          pUser = new User(*Security::getCurrentlyLoggedInUser());
        }
        m_IOServiceThread = boost::thread(boost::bind(&SocketHelper::runIOService, this, pUser));
      }
    }
  private:
    boost::shared_ptr<boost::asio::io_service> m_IOService;
    boost::thread m_IOServiceThread;
    boost::shared_ptr<boost::asio::io_service::work> m_pASIOWork;
    ScriptContext* m_pContext;
    boost::shared_ptr<ScriptObject> m_pCallbackObject;
    boost::shared_ptr<ScriptFunctionRooter> m_pFunctionRooter;
    jsval m_CallbackFunction;
    boost::shared_ptr<ScriptContextAttachedObject> m_pAttachedObject;
  }; // SocketHelper

  class SocketHelperInstance : public SocketHelper,
                               public boost::enable_shared_from_this<SocketHelperInstance> {
  public:
    SocketHelperInstance(SocketScriptContextExtension& _extension)
    : SocketHelper(_extension)
    { }

    ~SocketHelperInstance() {
      if(isConnected()) {
        close();
      }
      m_pSocket.reset();
      if(m_pAcceptor != NULL) {
        m_pAcceptor->close();
      }
      m_pAcceptor.reset();
    }

    void connectTo(const std::string& _host, int _port) {
      tcp::resolver resolver(getIOService());
      tcp::resolver::query query(_host, intToString(_port));
      try {
        tcp::resolver::iterator iterator = resolver.resolve(query);

        tcp::endpoint endpoint = *iterator;
        m_pSocket.reset(new tcp::socket(getIOService()));
        m_pSocket->async_connect(
          endpoint,
          boost::bind(
            &SocketHelperInstance::connectionCallback,
            this,
            boost::asio::placeholders::error,
            ++iterator));
        startBlockingCall();
      } catch(boost::system::system_error&) {
        Logger::getInstance()
          ->log("JS: connectTo: Could not resolve host '" + _host, lsWarning);
        failure();
      }
    }

    void send(const std::string& _data) {
      m_Data = _data;
      boost::asio::async_write(*m_pSocket,
        boost::asio::buffer(
          m_Data.c_str(),
          m_Data.size()),
        boost::bind(
          &SocketHelperInstance::sendCallback,
          this,
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred));
      startBlockingCall();
    }

    void close() {
      m_pSocket->close();
      endBlockingCall();
    }

    void receive(const int _numberOfBytes) {
      m_BytesToRead = std::min(_numberOfBytes, int(kMaxDataLength));
      boost::asio::async_read(
        *m_pSocket,
        boost::asio::buffer(
          m_DataBuffer,
          m_BytesToRead),
        boost::bind(&SocketHelperInstance::readCallback,
                    this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
      startBlockingCall();
    }

    void receiveLine(const int _maxBytes, const std::string& _delimiter) {
      m_BytesToRead = std::min(_maxBytes, int(kMaxDataLength));
      boost::asio::async_read_until(
        *m_pSocket,
        m_LineBuffer,
        _delimiter,
        boost::bind(&SocketHelperInstance::receiveLineCallback,
                    this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred,
                    _delimiter));
      startBlockingCall();
    }

    bool isConnected() {
      return (m_pSocket != NULL) && m_pSocket->is_open();
    }

    void bind(const int _port) {
      try {
        tcp::endpoint endpoint(tcp::v4(), _port);
        m_pAcceptor.reset(new tcp::acceptor(getIOService()));
        m_pAcceptor->open(endpoint.protocol());
        m_pAcceptor->set_option(tcp::acceptor::reuse_address(true));
        m_pAcceptor->bind(endpoint);
        m_pAcceptor->listen();
        success();
      } catch(boost::system::system_error& error) {
        Logger::getInstance()->log("SocketHelperInstance::bind: Caught exception: " + error.code().message(), lsFatal);
        failure();
      }
    }

    void accept() {
      if(m_pSocket == NULL && m_pAcceptor != NULL) {
        createSocket();
        m_pAcceptor->async_accept(*m_pSocket, boost::bind(&SocketHelperInstance::acceptCallback, this, boost::asio::placeholders::error));
        startBlockingCall();
      } else {
        Logger::getInstance()->log("SocketHelperInstance::accept: Please call bind first", lsFatal);
      }
    }

    virtual void stop() {
      if(m_pSocket != NULL) {
        boost::thread(boost::bind(&SocketHelperInstance::close, this));
      }
    }

  private:
    void connectionCallback(const boost::system::error_code& error,
                            tcp::resolver::iterator endpoint_iterator) {
      Logger::getInstance()->log("*** Connection callback");
      ScriptLock lock(&getContext());
      JSContextThread req(&getContext());
      boost::shared_ptr<ScriptContextAttachedObject> delayFreeing = endBlockingCall();

      if (!error) {
        success();
      } else if(endpoint_iterator != tcp::resolver::iterator()) {
        m_pSocket->close();
        tcp::endpoint endpoint = *endpoint_iterator;
        m_pSocket->async_connect(endpoint,
                                 boost::bind(&SocketHelperInstance::connectionCallback, this,
                                             boost::asio::placeholders::error, ++endpoint_iterator));
      } else {
        Logger::getInstance()->log("SocketHelperInstance::connectionCallback: failed: " + error.message());
        failure();
      }
      req.endRequest();
      //JS_ClearContextThread(getContext().getJSContext());
      Logger::getInstance()->log("*** Done: Connection callback");
    } // connectionCallback

    void sendCallback(const boost::system::error_code& error, std::size_t bytesTransfered) {
      Logger::getInstance()->log("*** Send callback");
      ScriptLock lock(&getContext());
      JSContextThread req(&getContext());
      boost::shared_ptr<ScriptContextAttachedObject> delayFreeing = endBlockingCall();
      if(!error) {
        if(bytesTransfered == m_Data.size()) {
          m_Data.clear();
          callSizeCallback(bytesTransfered);
        }
      } else {
        Logger::getInstance()->log("SocketHelperInstance::sendCallback: error: " + error.message());
        callSizeCallback(-1);
      }
      Logger::getInstance()->log("*** Done: Send callback");
    }

    void readCallback(const boost::system::error_code& error, std::size_t bytesTransfered) {
      ScriptLock lock(&getContext());
      JSContextThread req(&getContext());
      if(!error) {
        if(bytesTransfered == m_BytesToRead) {
          std::string result(m_DataBuffer, m_BytesToRead);
          m_BytesToRead = 0;
          boost::shared_ptr<ScriptContextAttachedObject> delayFreeing = endBlockingCall();
          callDataCallback(result);
        }
      } else {
        boost::shared_ptr<ScriptContextAttachedObject> delayFreeing = endBlockingCall();
        Logger::getInstance()->log("SocketHelperInstance::readCallback: error: " + error.message());
        callDataCallback("");
      }
    }

    void receiveLineCallback(const boost::system::error_code& _error, std::size_t _bytesTransfered, std::string _delimiter) {
      ScriptLock lock(&getContext());
      JSContextThread req(&getContext());
      if(!_error) {
        boost::asio::streambuf::const_buffers_type bufs = m_LineBuffer.data();
        std::string line(
            boost::asio::buffers_begin(bufs),
            boost::asio::buffers_begin(bufs) + _bytesTransfered);
        if(endsWith(line, _delimiter)) {
          line.erase(line.size() - _delimiter.size());
        }
        m_LineBuffer.consume(_bytesTransfered);
        boost::shared_ptr<ScriptContextAttachedObject> delayFreeing = endBlockingCall();
        callDataCallback(line);
      } else {
        boost::shared_ptr<ScriptContextAttachedObject> delayFreeing = endBlockingCall();
        Logger::getInstance()->log("SocketHelperInstance::receiveLineCallback: error: " + _error.message());
        callDataCallback("");
      }
    }

    void acceptCallback(const boost::system::error_code& error) {
      ScriptLock lock(&getContext());
      JSContextThread req(&getContext());
      boost::shared_ptr<ScriptContextAttachedObject> delayFreeing = endBlockingCall();
      if(!error) {
        if(hasCallback()) {
          JSObject* socketObj = JS_ConstructObject(getContext().getJSContext(), &tcpSocket_class, NULL, NULL);
          SocketHelperInstance* pInstance = static_cast<SocketHelperInstance*>(JS_GetPrivate(getContext().getJSContext(), socketObj));
          assert(pInstance != NULL);

          pInstance->m_pSocket = m_pSocket;
          pInstance->setIOService(getIOServicePtr());
          m_pSocket.reset();
          ScriptFunctionParameterList list(getContext());
          list.addJSVal(OBJECT_TO_JSVAL(socketObj));
          callCallbackWithArguments(list);
        }
      } else {
        Logger::getInstance()->log("SocketHelperInstance::acceptCallback: error: " + error.message());
        ScriptFunctionParameterList list(getContext());
        list.addJSVal(JSVAL_NULL);
        callCallbackWithArguments(list);
      }
    }

    void createSocket() {
      m_pSocket.reset(new tcp::socket(getIOService()));
    }

    void success() {
      callCallback(true);
    }

    void failure() {
      callCallback(false);
    }

    void callCallback(bool _result) {
      JSAutoLocalRootScope rootScope(getContext().getJSContext());
      ScriptFunctionParameterList param(getContext());
      param.add<bool>(_result);
      callCallbackWithArguments(param);
    }

    void callSizeCallback(int _result) {
      JSAutoLocalRootScope rootScope(getContext().getJSContext());
      ScriptFunctionParameterList param(getContext());
      param.add<int>(_result);
      callCallbackWithArguments(param);
    }

    void callDataCallback(const std::string& _result) {
      JSAutoLocalRootScope rootScope(getContext().getJSContext());
      ScriptFunctionParameterList param(getContext());
      param.add<std::string>(_result);
      callCallbackWithArguments(param);
    }
  private:
    boost::shared_ptr<tcp::socket> m_pSocket;
    std::string m_Data;
    enum { kMaxDataLength = 1024 };
    char m_DataBuffer[kMaxDataLength];
    std::size_t m_BytesToRead;
    boost::shared_ptr<tcp::acceptor> m_pAcceptor;
    boost::asio::streambuf m_LineBuffer;
  }; // SocketHelperInstance

  class SocketHelperSendOneShot : public SocketHelper,
                                  public boost::enable_shared_from_this<SocketHelperSendOneShot> {
  public:
    SocketHelperSendOneShot(SocketScriptContextExtension& _extension,
                            ScriptContext* _pContext)
    : SocketHelper(_extension)
    {
      Logger::getInstance()->log("Creating SocketHelperSendOneShot");
      setContext(_pContext);
      m_pSocket.reset(new tcp::socket(getIOService()));
    }

    ~SocketHelperSendOneShot() {
      Logger::getInstance()->log("Destroying SocketHelperSendOneShot");
      if(m_pSocket->is_open()) {
        m_pSocket->close();
      }
      m_pSocket.reset();
      Logger::getInstance()->log("Done: Destroying SocketHelperSendOneShot");
    }

    void sendTo(const std::string& _host, int _port, const std::string& _data) {
      Logger::getInstance()->log("SocketHelperSendOneShot::sendTo");
      m_Data = _data;
      tcp::resolver resolver(getIOService());
      tcp::resolver::query query(_host, intToString(_port));
      tcp::resolver::iterator iterator = resolver.resolve(query);

      tcp::endpoint endpoint = *iterator;
      m_pSocket->async_connect(endpoint,
          boost::bind(&SocketHelperSendOneShot::handle_connect, this,
            boost::asio::placeholders::error, ++iterator));
      startBlockingCall();
    }

    void write()
    {
      Logger::getInstance()->log("SocketHelperSendOneShot::write");
      boost::asio::async_write(*m_pSocket,
          boost::asio::buffer(m_Data.c_str(),
            m_Data.size()),
          boost::bind(&SocketHelperSendOneShot::handle_write, this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
    }

    virtual void stop() {
      if(m_pSocket != NULL) {
        do_close();
      }
      endBlockingCall();
    }

  private:
    void handle_connect(const boost::system::error_code& error,
        tcp::resolver::iterator endpoint_iterator)
    {
      Logger::getInstance()->log("SocketHelperSendOneShot::handle_connect");
      if (!error) {
        getIOService().post(boost::bind(&SocketHelperSendOneShot::write, this));
      } else if (endpoint_iterator != tcp::resolver::iterator()) {
        m_pSocket->close();
        tcp::endpoint endpoint = *endpoint_iterator;
        m_pSocket->async_connect(endpoint,
            boost::bind(&SocketHelperSendOneShot::handle_connect, this,
              boost::asio::placeholders::error, ++endpoint_iterator));
      } else {
        do_close();
        boost::thread(boost::bind(&SocketScriptContextExtension::removeSocketHelper, &m_Extension, shared_from_this()));
      }
    }

    void handle_write(const boost::system::error_code& error, std::size_t bytesTransfered) {
      Logger::getInstance()->log("SocketHelperSendOneShot::handle_write");
      bool finished = false;
      if(!error) {
        if(bytesTransfered == m_Data.size()) {
          finished = true;
        } else {
          return;
        }
      } else {
        Logger::getInstance()->log("SocketHelperSendOneShot::handle_write: " + error.message());
        finished = true;
      }
      if(finished) {
        if(hasCallback()) {
          ScriptLock lock(&getContext());
          JSContextThread req(&getContext());
          ScriptFunctionParameterList params(getContext());
          params.add<bool>(!error);
          callCallbackWithArguments(params);
        }
        do_close();
        boost::thread(boost::bind(&SocketScriptContextExtension::removeSocketHelper, &m_Extension, shared_from_this()));
      }
    }

    void do_close() {
      m_pSocket->shutdown(boost::asio::socket_base::shutdown_both);
      m_pSocket->close();
      endBlockingCall();
    }
  private:
    boost::shared_ptr<tcp::socket> m_pSocket;
    std::string m_Data;
  }; // SocketHelperOneShot

  //================================================== SocketScriptContextExtension

  const std::string SocketScriptExtensionName = "socketextension";

  SocketScriptContextExtension::SocketScriptContextExtension()
  : ScriptExtension(SocketScriptExtensionName)
  { }

  void delayedDestroy(SocketHelperInstance* _inst) {
    delete _inst;
  }

  void tcpSocket_finalize(JSContext *cx, JSObject *obj) {
    SocketHelperInstance* pInstance = static_cast<SocketHelperInstance*>(JS_GetPrivate(cx, obj));
    if(pInstance != NULL) {
      Logger::getInstance()->log("Finalizing Socket");
      JS_SetPrivate(cx, obj, NULL);
      pInstance->releaseCallbackObjects();
      boost::thread(boost::bind(&delayedDestroy, pInstance));
    }
  } // tcpSocket_finalize

  JSBool tcpSocket_construct(JSContext *cx, JSObject *obj, uintN argc,
                             jsval *argv, jsval *rval) {
    Logger::getInstance()->log("*** Constructing socket");
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    SocketScriptContextExtension* ext =
      dynamic_cast<SocketScriptContextExtension*>(ctx->getEnvironment().getExtension(SocketScriptExtensionName));
    SocketHelperInstance* inst = new SocketHelperInstance(*ext);
    JS_SetPrivate(cx, obj, inst);
    return JS_TRUE;
  } // tcpSocket_construct

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

  JSBool tcpSocket_receive(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    JSRequest req(cx);
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    SocketHelperInstance* pInst = static_cast<SocketHelperInstance*>(JS_GetPrivate(cx, obj));
    assert(pInst != NULL);
    if(pInst->isConnected()) {
      if(argc >= 1) {
        try {
          int size = ctx->convertTo<int>(argv[0]);

          pInst->setContext(ctx);

          // check if we've been given a callback
          if(argc >= 2) {
            boost::shared_ptr<ScriptObject> scriptObj(new ScriptObject(obj, *ctx));
            pInst->setCallbackObject(scriptObj);
            pInst->setCallbackFunction(argv[1]);
          }

          pInst->receive(size);
          *rval = JSVAL_TRUE;
          return JS_TRUE;
        } catch(const ScriptException& e) {
          Logger::getInstance()->log(std::string("tcpSocket_receive: Caught script exception: ") + e.what());
        }
      }
    } else {
      Logger::getInstance()->log("tcpSocket_receive: not connected, please call connect first", lsError);
    }
    return JS_FALSE;
  } // tcpSocket_receive

  JSBool tcpSocket_receiveLine(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    JSRequest req(cx);
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    SocketHelperInstance* pInst = static_cast<SocketHelperInstance*>(JS_GetPrivate(cx, obj));
    assert(pInst != NULL);
    if(pInst->isConnected()) {
      if(argc >= 1) {
        try {
          int size = ctx->convertTo<int>(argv[0]);
          std::string delimiter = "\r\n";

          pInst->setContext(ctx);

          // check if we've been given a callback
          if(argc >= 2) {
            boost::shared_ptr<ScriptObject> scriptObj(new ScriptObject(obj, *ctx));
            pInst->setCallbackObject(scriptObj);
            pInst->setCallbackFunction(argv[1]);
          }

          if(argc >= 3) {
            delimiter = ctx->convertTo<std::string>(argv[2]);
          }

          pInst->receiveLine(size, delimiter);
          *rval = JSVAL_TRUE;
          return JS_TRUE;
        } catch(const ScriptException& e) {
          Logger::getInstance()->log(std::string("tcpSocket_receiveLine: Caught script exception: ") + e.what());
        }
      }
    } else {
      Logger::getInstance()->log("tcpSocket_receiveLine: not connected, please call connect first", lsError);
    }
    return JS_FALSE;
  } // tcpSocket_receiveLine

  JSBool tcpSocket_bind(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    JSRequest req(cx);
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    SocketHelperInstance* pInst = static_cast<SocketHelperInstance*>(JS_GetPrivate(cx, obj));
    assert(pInst != NULL);
    if(argc >= 1) {
      try {
        int port = ctx->convertTo<int>(argv[0]);

        pInst->setContext(ctx);

        // check if we've been given a callback
        if(argc >= 2) {
          boost::shared_ptr<ScriptObject> scriptObj(new ScriptObject(obj, *ctx));
          pInst->setCallbackObject(scriptObj);
          pInst->setCallbackFunction(argv[1]);
        }

        pInst->bind(port);
        *rval = JSVAL_TRUE;
        return JS_TRUE;
      } catch(const ScriptException& e) {
        Logger::getInstance()->log(std::string("tcpSocket_bind: Caught script exception: ") + e.what());
      }
    }
    return JS_FALSE;
  } // tcpSocket_bind

  JSBool tcpSocket_accept(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    JSRequest req(cx);
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    SocketHelperInstance* pInst = static_cast<SocketHelperInstance*>(JS_GetPrivate(cx, obj));
    assert(pInst != NULL);
    try {
      pInst->setContext(ctx);

      // check if we've been given a callback
      if(argc >= 1) {
        boost::shared_ptr<ScriptObject> scriptObj(new ScriptObject(obj, *ctx));
        pInst->setCallbackObject(scriptObj);
        pInst->setCallbackFunction(argv[0]);
      }

      pInst->accept();
      *rval = JSVAL_TRUE;
      return JS_TRUE;
    } catch(const ScriptException& e) {
      Logger::getInstance()->log(std::string("tcpSocket_accept: Caught script exception: ") + e.what());
    }
    return JS_FALSE;
  } // tcpSocket_accept

  JSBool tcpSocket_close(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    JSRequest req(cx);
    SocketHelperInstance* pInst = static_cast<SocketHelperInstance*>(JS_GetPrivate(cx, obj));
    assert(pInst != NULL);
    pInst->close();
    *rval = JSVAL_TRUE;
    return JS_TRUE;
  } // tcpSocket_connect

  JSFunctionSpec tcpSocket_methods[] = {
    {"send", tcpSocket_send, 2, 0, 0},
    {"connect", tcpSocket_connect, 3, 0, 0},
    {"receive", tcpSocket_receive, 2, 0, 0},
    {"receiveLine", tcpSocket_receiveLine, 2, 0, 0},
    {"bind", tcpSocket_bind, 2, 0, 0},
    {"accept", tcpSocket_accept, 1, 0, 0},
    {"close", tcpSocket_close, 0, 0, 0},
    {NULL, NULL, 0, 0, 0},
  }; // tcpSockets_methods

  JSBool tcpSocket_sendTo(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    JSRequest req(cx);
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    SocketScriptContextExtension* ext =
       dynamic_cast<SocketScriptContextExtension*>(ctx->getEnvironment().getExtension(SocketScriptExtensionName));
    assert(ext != NULL);

    if(argc >= 3) {
      try {
        std::string host = ctx->convertTo<std::string>(argv[0]);
        int port = ctx->convertTo<int>(argv[1]);
        std::string data = ctx->convertTo<std::string>(argv[2]);

        boost::shared_ptr<SocketHelperSendOneShot> helper(new SocketHelperSendOneShot(*ext, ctx));
        ext->addSocketHelper(helper);

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
    JSRequest req(cx);
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
    JSRequest req(&_context);
    JS_InitClass(_context.getJSContext(), _context.getRootObject().getJSObject(),
                 NULL, &tcpSocket_class, tcpSocket_construct, 0, tcpSocket_properties,
                 tcpSocket_methods, NULL, tcpSocket_static_methods);
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
