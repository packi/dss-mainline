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


#include "ds485scriptextension.h"

#include <iostream>

#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include "scriptobject.h"
#include "core/ds485/ds485.h"
#include "core/ds485/framebucketcollector.h"
#include "core/ds485const.h"
#include "core/DS485Interface.h"

namespace dss {

  const std::string DS485ScriptExtensionName = "ds485extension";

  static JSClass DS485Frame_class = {
    "DS485Frame", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,  JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStandardClasses,
    JS_ResolveStub,
    JS_ConvertStub,  JS_FinalizeStub, JSCLASS_NO_OPTIONAL_MEMBERS
  }; // DS485Frame_class

  DS485ScriptExtension::DS485ScriptExtension(FrameSenderInterface& _frameSender, FrameBucketHolder& _frameBucketHolder)
  : ScriptExtension(DS485ScriptExtensionName),
    m_FrameSender(_frameSender),
    m_FrameBucketHolder(_frameBucketHolder)
  {
  } // ctor

  void DS485ScriptExtension::sendFrame(ScriptObject& _frame) {
    boost::shared_ptr<DS485CommandFrame> cmdFrame = frameFromScriptObject(_frame);
    m_FrameSender.sendFrame(*cmdFrame);
  } // sendFrame

  void DS485ScriptExtension::sendFrame(ScriptObject& _frame, boost::shared_ptr<ScriptObject> _callbackObject, jsval _function, int _timeout) {
    boost::shared_ptr<DS485CommandFrame> cmdFrame = frameFromScriptObject(_frame);
    if(cmdFrame->getPayload().size() > 0) {
      int sourceID = cmdFrame->getHeader().isBroadcast() ? -1 : cmdFrame->getHeader().getDestination();
      int functionID = cmdFrame->getPayload().toChar()[0];
      setCallback(sourceID, functionID, _callbackObject, _function, _timeout);
    } else {
      Logger::getInstance()->log("DS485ScriptExtension::sendFrame: not adding a callback because the payload has a size of zero");
    }
    m_FrameSender.sendFrame(*cmdFrame);
  } // sendFrame

  class DS485ScriptFrameCatcher : public ScriptContextAttachedObject {
  public:
    DS485ScriptFrameCatcher(ScriptContext* _pContext, DS485ScriptExtension* _pExtension)
    : ScriptContextAttachedObject(_pContext),
      m_pExtension(_pExtension)
    {
    }

    void waitForFrame(boost::shared_ptr<FrameBucketCollector> _bucket, boost::shared_ptr<ScriptObject> _callbackObject, jsval _function, int _timeout, boost::shared_ptr<ScriptFunctionRooter> _rootedFunction) {
      bool goOn = true;
      JSContext* cx = getContext()->getJSContext();
      do {
        bool hasFrame = _bucket->waitForFrame(_timeout);
        try {
          boost::shared_ptr<DS485CommandFrame> frame = _bucket->popFrame();
          AssertLocked contextLock(getContext());
          Logger::getInstance()->log("waitForFrame: got frame, aquiring request");
          JS_SetContextThread(cx);
          JSRequest req(cx);
          Logger::getInstance()->log("waitForFrame: got frame, got request");
          {
            JSAutoLocalRootScope scope(cx);
            ScriptFunctionParameterList paramList(_callbackObject->getContext());
            if(hasFrame) {
              JSObject* frameJSObj = JS_NewObject(cx, &DS485Frame_class, NULL, _callbackObject->getJSObject());
              ScriptObject frameObj(frameJSObj, _callbackObject->getContext());
              frameObj.setProperty("broadcast", frame->getHeader().isBroadcast());
              frameObj.setProperty<int>("source", frame->getHeader().getSource());
              frameObj.setProperty<int>("destination", frame->getHeader().getDestination());
              PayloadDissector pd(frame->getPayload());
              if(!pd.isEmpty()) {
                int functionID = pd.get<uint8_t>();
                frameObj.setProperty("functionID", functionID);
                ScriptObject payloadObj(JS_NewArrayObject(cx, 0, NULL), _callbackObject->getContext());
                frameObj.setProperty("payload", &payloadObj);
                int iElem = 0;
                while(!pd.isEmpty()) {
                  int val = pd.get<uint16_t>();
                  jsval jval = INT_TO_JSVAL(val);
                  JS_SetElement(cx, payloadObj.getJSObject(), iElem, &jval);
                  iElem++;
                }
              }
              paramList.addJSVal(OBJECT_TO_JSVAL(frameJSObj));
            } else {
              paramList.addJSVal(JSVAL_NULL);
            }
            goOn = _callbackObject->callFunctionByReference<bool>(_function, paramList);
          }
          //JS_GC(cx);
          req.endRequest();
          JS_ClearContextThread(cx);
        } catch(ScriptException& e) {
          Logger::getInstance()->log(std::string("DS485ScriptExtension::waitForFrame: Caught exception: ") + e.what(), lsError);
          goOn = false;
        }
      } while(goOn);
      m_pExtension->removeCallback(_bucket);
      delete this;
    } // waitForFrame

  private:
    DS485ScriptExtension* m_pExtension;
  };

  void DS485ScriptExtension::setCallback(int _sourceID, int _functionID, boost::shared_ptr<ScriptObject> _callbackObject, jsval _function, int _timeout) {
    boost::shared_ptr<FrameBucketCollector> frameBucket(new FrameBucketCollector(&m_FrameBucketHolder, _functionID, _sourceID), FrameBucketBase::removeFromHolderAndDelete);
    frameBucket->addToHolder();
    m_CallbackMutex.lock();
    m_Callbacks.push_back(frameBucket);
    m_CallbackMutex.unlock();
    boost::shared_ptr<ScriptFunctionRooter> rootedFunction(new ScriptFunctionRooter(&_callbackObject->getContext(), _callbackObject->getJSObject(), _function));
    DS485ScriptFrameCatcher* catcher = new DS485ScriptFrameCatcher(&_callbackObject->getContext(), this);
    boost::thread(boost::bind(&DS485ScriptFrameCatcher::waitForFrame, catcher, frameBucket, _callbackObject, _function, _timeout, rootedFunction));
  } // setCallback

  boost::shared_ptr<DS485CommandFrame> DS485ScriptExtension::frameFromScriptObject(ScriptObject& _object) {
    boost::shared_ptr<DS485CommandFrame> result(new DS485CommandFrame);
    result->getHeader().setBroadcast(_object.getProperty<bool>("broadcast"));
    result->getHeader().setSource(_object.getProperty<int>("source"));
    result->getHeader().setDestination(_object.getProperty<int>("destination"));
    result->setCommand(_object.getProperty<int>("command"));
    int functionID = _object.getProperty<int>("functionID");
    result->getPayload().add<uint8_t>(functionID);
    jsval arrVal = _object.doGetProperty("payload");
    if(JSVAL_IS_OBJECT(arrVal)) {
      JSObject* arrObj = JSVAL_TO_OBJECT(arrVal);
      JSIdArray* array = JS_Enumerate(_object.getContext().getJSContext(), arrObj);
      if(array != NULL) {
        for(int iElement = 0; iElement < array->length; iElement++) {
          jsid id = array->vector[iElement];
          JSObject* obj;
          jsval vp;
          JS_GetMethodById(_object.getContext().getJSContext(), arrObj, id, &obj, &vp);

          int param = _object.getContext().convertTo<int>(vp);
          result->getPayload().add<uint16_t>(param & 0xFFFF);
        }
      }
    }
    return result;
  } // frameFromScriptObject

  JSBool ds485_sendFrame(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    DS485ScriptExtension* ext =
       dynamic_cast<DS485ScriptExtension*>(ctx->getEnvironment().getExtension(DS485ScriptExtensionName));
    assert(ext != NULL);

    if(argc >= 1) {
      try {
        jsval frameObj = argv[0];
        if(!JSVAL_IS_OBJECT(frameObj)) {
          Logger::getInstance()->log("Expected an object as the first parameter");
          return JS_FALSE;
        }
        ScriptObject frameScriptObj(JSVAL_TO_OBJECT(frameObj), *ctx);
        jsval func = JSVAL_NULL;
        if(argc >= 2) {
          func = argv[1];
        }
        int timeout = 1000;
        if(argc >= 3) {
          Logger::getInstance()->log("Converting to int");
          if(JSVAL_IS_NUMBER(argv[2])) {
            timeout = ctx->convertTo<int>(argv[2]);
          }
          Logger::getInstance()->log("Done converting to int");
        }

        if((func != JSVAL_NULL) && (func != JSVAL_VOID)) {
          boost::shared_ptr<ScriptObject> callbackObj(new ScriptObject(obj, *ctx));
          ext->sendFrame(frameScriptObj, callbackObj, func, timeout);
        } else {
          ext->sendFrame(frameScriptObj);
        }

        *rval = JSVAL_TRUE;
        return JS_TRUE;
      } catch(const ScriptException& e) {
        Logger::getInstance()->log(std::string("ds485_sendFrame: Caught script exception: ") + e.what());
      }
    }
    return JS_FALSE;
  } // ds485_sendFrame

  JSBool ds485_setCallback(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    DS485ScriptExtension* ext =
    dynamic_cast<DS485ScriptExtension*>(ctx->getEnvironment().getExtension(DS485ScriptExtensionName));
    assert(ext != NULL);

    if(argc >= 2) {
      try {
        jsval func = argv[0];
        int functionID = ctx->convertTo<int>(argv[1]);
        int timeout = 1000;
        if(argc >= 3) {
          timeout = ctx->convertTo<int>(argv[2]);
        }
        int source = -1;
        if(argc >= 4) {
          source = ctx->convertTo<int>(argv[3]);
        }

        boost::shared_ptr<ScriptObject> callbackObj(new ScriptObject(obj, *ctx));
        ext->setCallback(source, functionID, callbackObj, func, timeout);

        *rval = JSVAL_TRUE;
        return JS_TRUE;
      } catch(const ScriptException& e) {
        Logger::getInstance()->log(std::string("ds485_sendFrame: Caught script exception: ") + e.what());
      }
    }
    return JS_FALSE;
  } // ds485_setCallback

  JSFunctionSpec DS485_static_methods[] = {
    {"sendFrame", ds485_sendFrame, 3, 0, 0},
    {"setCallback", ds485_setCallback, 2, 0, 0},
    {NULL, NULL, 0, 0, 0},
  }; // DS485_static_methods

  static JSClass DS485_class = {
    "DS485", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,  JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStandardClasses,
    JS_ResolveStub,
    JS_ConvertStub,  JS_FinalizeStub, JSCLASS_NO_OPTIONAL_MEMBERS
  }; // DS485_class

  JSBool DS485_construct(JSContext *cx, JSObject *obj, uintN argc,
                              jsval *argv, jsval *rval) {
    return JS_FALSE;
  } // DS485_construct

  JSBool DS485Frame_construct(JSContext *cx, JSObject *obj, uintN argc,
                              jsval *argv, jsval *rval) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    ScriptObject objWrapper(obj, *ctx);
    objWrapper.setProperty("source", -1);
    objWrapper.setProperty("broadcast", false);
    objWrapper.setProperty("destination", -1);
    ScriptObject payloadObj(JS_NewArrayObject(cx, 0, NULL), *ctx);
    objWrapper.setProperty("payload", &payloadObj);
    objWrapper.setProperty("functionID", -1);
    objWrapper.setProperty<int>("command", CommandRequest);
    return JS_TRUE;
  } // DS485Frame_construct

  void DS485ScriptExtension::extendContext(ScriptContext& _context) {
    JS_InitClass(_context.getJSContext(), _context.getRootObject().getJSObject(),
                 NULL, &DS485_class, DS485_construct, 0, NULL,
                 NULL, NULL, DS485_static_methods);
    JS_InitClass(_context.getJSContext(), _context.getRootObject().getJSObject(),
                NULL, &DS485Frame_class, DS485Frame_construct, 0, NULL,
                NULL, NULL, NULL);
  } // extendContext

  void DS485ScriptExtension::removeCallback(boost::shared_ptr<FrameBucketCollector> _pCallback) {
    m_CallbackMutex.lock();
    CallbackVector::iterator it = std::find(m_Callbacks.begin(), m_Callbacks.end(), _pCallback);
    if(it != m_Callbacks.end()) {
      m_Callbacks.erase(it);
    }
    m_CallbackMutex.unlock();
  } // removeCallback

} // namespace dss
