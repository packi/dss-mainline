/*
    Copyright (c) 2014 digitalSTROM AG, Zurich, Switzerland

    Author: Sergey Bostandzhyan <jin@dev.digitalstrom.org>

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

#include "config.h"

#ifdef HAVE_CURL

#include <jsapi.h>
#include <boost/make_shared.hpp>

#include "security/security.h"
#include "security/user.h"
#include "propertysystem.h"
#include "scriptobject.h"
#include "jswebservice.h"
#include "webservice_connection.h"
#include <string.h>

namespace dss {

  const std::string WebserviceConnectionScriptContextExtensionName = "webserviceextension";

  WebserviceConnectionScriptContextExtension::WebserviceConnectionScriptContextExtension() : ScriptExtension(WebserviceConnectionScriptContextExtensionName)
  {}

  class WebserviceRequestCallback : public URLRequestCallback,
                                    public ScriptContextAttachedObject
  {
  public:
    ScriptFunctionRooter* m_ScriptRoot;
    ScriptContext* m_pContext;
    JSObject* m_pFunctionObject;
    jsval m_Function;
    boost::shared_ptr<ScriptObject> m_pScriptObject;
    User* m_pRunAsUser;
  public:
    WebserviceRequestCallback(ScriptContext* _context, ScriptFunctionRooter* _rooter, JSObject *_functionObj, jsval _function ) :
      ScriptContextAttachedObject(_context),
      m_pContext(_context),
      m_pFunctionObject(_functionObj),
      m_Function(_function),
      m_ScriptRoot(_rooter),
      m_pRunAsUser(NULL)
      {
        if(Security::getCurrentlyLoggedInUser() != NULL) {
          m_pRunAsUser = new User(*Security::getCurrentlyLoggedInUser());
        }
      }
    virtual ~WebserviceRequestCallback()
    {
        if (m_pRunAsUser) {
          delete m_pRunAsUser;
        }
    }
    virtual void result(long code, boost::shared_ptr<URLResult> result) {
      if (code != 200) {
        Logger::getInstance()->log(std::string(__PRETTY_FUNCTION__) +
            " HTTP request failed " + intToString(code), lsError);
      }

      if (getIsStopped()) {
        ScriptLock lock(m_pContext);
        JSContextThread req(m_pContext);
        delete m_ScriptRoot;
        return;
      }

      if(m_pRunAsUser != NULL) {
        Security* pSecurity = m_pContext->getEnvironment().getSecurity();
        if(pSecurity != NULL) {
          pSecurity->signIn(m_pRunAsUser);
        }
      }

      {
        ScriptLock lock(m_pContext);
        JSContextThread req(m_pContext);

        if ((m_pFunctionObject) && (!JSVAL_IS_NULL(m_Function))) {
          m_pScriptObject.reset(new ScriptObject(m_pFunctionObject, *getContext()));
          ScriptFunctionParameterList list(*m_pContext);
          list.add<int>((int)code);
          list.add<std::string>(result->content());

          try {
            m_pScriptObject->callFunctionByReference<void>(m_Function, list);
          } catch(ScriptException& e) {
            Logger::getInstance()->log("JavaScript: error running webservice callback: " +
                std::string(e.what()), lsFatal);
          }
        }
        m_pScriptObject.reset();
        delete m_ScriptRoot;
      }
    }
  };

  /* url, request type, callback */
  static JSBool webservice_simple_request(JSContext *cx, uintN argc, jsval *vp)
  {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    RequestType reqtype;
    JSObject *jsCallback = NULL;
    jsval functionVal = JSVAL_NULL;
    std::string url;
    std::string req = "GET";

    /* for some reason JS_ConvertArguments() ALWAYS returned false so had to
     * avoid using it */
    if (argc >= 1) {
      if (JSVAL_IS_STRING(JS_ARGV(cx, vp)[0])) {
        url = ctx->convertTo<std::string>(JS_ARGV(cx, vp)[0]);
      } else {
        JS_ReportError(cx, "simplerequest(): invalid url parameter",
                       lsError);
        return JS_FALSE;
      }
    }

    if (url.empty()) {
      JS_ReportError(cx, "simplerequest(): empty url parameter", lsError);
      return JS_FALSE;
    }

    if (argc >= 2) {
      if (JSVAL_IS_STRING(JS_ARGV(cx, vp)[1])) {
        req = ctx->convertTo<std::string>(JS_ARGV(cx, vp)[1]);
      }
    }

    if (req ==  "GET") {
      reqtype = GET;
    } else if (req == "POST") {
      reqtype = POST;
    } else {
      JS_ReportError(cx, "simplerequest(): request type must be 'GET' or "
                     "'POST", lsError);
      return JS_FALSE;
    }

    if (argc >= 3) {
      if (JSVAL_IS_OBJECT(JS_ARGV(cx, vp)[2])) {
        if (!JS_ObjectIsCallable(cx, JSVAL_TO_OBJECT(JS_ARGV(cx, vp)[2]))) {
          JS_ReportError(cx, "simplerequest(): invalid callback parameter",
                         lsError);
          return JS_FALSE;
        }

        jsCallback = JSVAL_TO_OBJECT(JS_ARGV(cx, vp)[2]);
        functionVal = JS_ARGV(cx, vp)[2];
      } else {
        JS_ReportError(cx, "simplerequest(): invalid callback parameter",
                       lsError);
        return JS_FALSE;
      }
    }

    ScriptFunctionRooter* fRoot = new ScriptFunctionRooter(ctx, jsCallback, functionVal);

    boost::shared_ptr<WebserviceRequestCallback> mcb(
        new WebserviceRequestCallback(ctx, fRoot, jsCallback, functionVal));

    WebserviceConnection::getInstance()->request(url, reqtype, mcb);

    return JS_TRUE;
  }

  static JSBool webservice_request(JSContext *cx, uintN argc, jsval *vp)
  {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));
    RequestType reqtype;
    JSObject *jsCallback = NULL;
    jsval functionVal = JSVAL_NULL;
    std::string url;
    std::string req = "GET";
    HashMapStringString headers;
    HashMapStringString formpost;

    /* for some reason JS_ConvertArguments() ALWAYS returned false so had to
     * avoid using it */
    if (argc >= 1) {
        if (JSVAL_IS_STRING(JS_ARGV(cx, vp)[0])) {
          url = ctx->convertTo<std::string>(JS_ARGV(cx, vp)[0]);
        } else {
          JS_ReportError(cx, "request(): invalid url parameter", lsError);
          return JS_FALSE;
        }
    }

    if (url.empty()) {
      JS_ReportError(cx, "request(): empty url parameter", lsError);
      return JS_FALSE;
    }

    if (argc >= 2) {
      if (JSVAL_IS_STRING(JS_ARGV(cx, vp)[1])) {
        req = ctx->convertTo<std::string>(JS_ARGV(cx, vp)[1]);
      }
    }

    if (req ==  "GET") {
      reqtype = GET;
    } else if (req == "POST") {
      reqtype = POST;
    } else {
      JS_ReportError(cx, "simplerequest(): request type must be 'GET' or "
                     "'POST", lsError);
      return JS_FALSE;
    }

    // headers hashmap, may be null
    if ((argc >= 3) && (!JSVAL_IS_NULL(JS_ARGV(cx, vp)[2]))) {
      if (JSVAL_IS_OBJECT(JS_ARGV(cx, vp)[2])) {
        JSObject *obj = JSVAL_TO_OBJECT(JS_ARGV(cx, vp)[2]);
        JSIdArray *ids = JS_Enumerate(cx, obj);
        if (ids) {
          for (jsint i = 0; i < ids->length; i++) {
            jsid id = ids->vector[i];
            jsval key;
            if (!JS_IdToValue(cx, id, &key)) {
              JS_ReportError(cx, "request(): could not parse header key",
                            lsWarning);
              continue;
            }

            if (!JSVAL_IS_STRING(key)) {
              JS_ReportError(cx, "request(): could not parse header key",
                            lsWarning);
              continue;
            }

            std::string name = ctx->convertTo<std::string>(key);
            if (name.empty()) {
              JS_ReportError(cx, "request(): could not parse header key",
                            lsWarning);
              continue;
            }

            jsval value;
            if (!JS_GetProperty(cx, obj, name.c_str(), &value)) {
              JS_ReportError(cx, "request(): could not parse header value",
                            lsWarning);
              continue;
            }

            if (!JSVAL_IS_STRING(value)) {
              JS_ReportError(cx, "request(): could not parse header value",
                            lsWarning);
              continue;
            }

            headers[name] = ctx->convertTo<std::string>(value);
          }
        }
        JS_DestroyIdArray(cx, ids);
      } else {
        JS_ReportError(cx, "simplerequest(): invalid headers parameter",
                       lsError);
        return JS_FALSE;
      }
    }

    // formpost hashmap
    if ((argc >= 4) && (!JSVAL_IS_NULL(JS_ARGV(cx, vp)[3]))) {
      if (JSVAL_IS_OBJECT(JS_ARGV(cx, vp)[3])) {
        JSObject *obj = JSVAL_TO_OBJECT(JS_ARGV(cx, vp)[3]);
        JSIdArray *ids = JS_Enumerate(cx, obj);
        if (ids) {
          for (jsint i = 0; i < ids->length; i++) {
            jsid id = ids->vector[i];
            jsval key;
            if (!JS_IdToValue(cx, id, &key)) {
              JS_ReportError(cx, "request(): could not parse formpost key",
                            lsWarning);
              continue;
            }

            if (!JSVAL_IS_STRING(key)) {
              JS_ReportError(cx, "request(): could not parse formpost key",
                            lsWarning);
              continue;
            }

            std::string name = ctx->convertTo<std::string>(key);
            if (name.empty()) {
              JS_ReportError(cx, "request(): could not parse formpost key",
                            lsWarning);
              continue;
            }

            jsval value;
            if (!JS_GetProperty(cx, obj, name.c_str(), &value)) {
              JS_ReportError(cx, "request(): could not parse formpost value",
                            lsWarning);
              continue;
            }

            if (!JSVAL_IS_STRING(value)) {
              JS_ReportError(cx, "request(): could not parse formpost value",
                            lsWarning);
              continue;
            }

            formpost[name] = ctx->convertTo<std::string>(value);
          }
        }
        JS_DestroyIdArray(cx, ids);
      } else {
        JS_ReportError(cx, "simplerequest(): invalid formpost parameter",
                       lsError);
        return JS_FALSE;
      }
    }

    // callback
    if (argc >= 5) {
      if (JSVAL_IS_OBJECT(JS_ARGV(cx, vp)[4])) {
        if (!JS_ObjectIsCallable(cx, JSVAL_TO_OBJECT(JS_ARGV(cx, vp)[4]))) {
          JS_ReportError(cx, "simplerequest(): invalid callback parameter",
                         lsError);
          return JS_FALSE;
        }

        jsCallback = JSVAL_TO_OBJECT(JS_ARGV(cx, vp)[4]);
        functionVal = JS_ARGV(cx, vp)[4];
      } else {
        JS_ReportError(cx, "simplerequest(): invalid callback parameter",
                       lsError);
        return JS_FALSE;
      }
    }

    ScriptFunctionRooter* fRoot = new ScriptFunctionRooter(ctx, jsCallback, functionVal);

    boost::shared_ptr<WebserviceRequestCallback> mcb(
        new WebserviceRequestCallback(ctx, fRoot, jsCallback, functionVal));

    WebserviceConnection::getInstance()->request(url, reqtype,
                            boost::make_shared<HashMapStringString>(headers),
                            boost::make_shared<HashMapStringString>(formpost),
                            mcb);

    return JS_TRUE;
  }

  static JSClass WebserviceConnection_class =
  {
    "WebserviceConnection", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStandardClasses,
    JS_ResolveStub,
    JS_ConvertStub,
    NULL,
    JSCLASS_NO_OPTIONAL_MEMBERS
  };

  static JSFunctionSpec WebserviceConnection_methods[] =
  {
      JS_FS("simplerequest", webservice_simple_request, 3, 0),
      JS_FS("request", webservice_request, 5, 0),
      JS_FS_END
  };

  JSBool WebserviceConnection_construct(JSContext *cx, uintN argc, jsval *vp) {
    ScriptContext *ctx = static_cast<ScriptContext *>(JS_GetContextPrivate(cx));
    JSRequest req(ctx);
    JSObject *obj = JS_NewObject(cx, &WebserviceConnection_class, NULL, NULL);
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(obj));
    return JS_TRUE;
  }

  void WebserviceConnectionScriptContextExtension::extendContext(ScriptContext& _context)
  {
    JS_InitClass(
        _context.getJSContext(),
        _context.getRootObject().getJSObject(),
        NULL, /* prototype */
        &WebserviceConnection_class, /* class */
        WebserviceConnection_construct, 0, /* constructor + arguments */
        NULL, /* properties */
        WebserviceConnection_methods, /* functions */
        NULL, /* static_ps */
        NULL); /* static_fs */
  }

} // namespace

#endif

