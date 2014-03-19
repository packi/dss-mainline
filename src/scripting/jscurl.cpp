/*
    Copyright (c) 2012 digitalSTROM.org, Zurich, Switzerland

    Author: Michael Tross, aizo GmbH <michael.tross@aizo.com>

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
/*
   The Initial Developer of the Original Code is Nick Galbreath

   Portions created by the Initial Developer are
   Copyright (c) 2009-2010, Nick Galbreath. All Rights Reserved.

   http://code.google.com/p/gpsee/
*/

#include <curl/curl.h>
#include <jsapi.h>

#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include "jscurl.h"
#include "jscurl-constants.h"
#include "jscurl-options.h"
#include "src/scripting/scriptobject.h"
#include "src/dss.h"
#include "src/propertysystem.h"
#include "src/security/user.h"
#include "src/security/security.h"

namespace dss {

  const std::string CurlScriptExtensionName = "curlextension";

  static JSClass* easycurl_slist_class;
  static JSObject* easycurl_slist_proto;

  static JSClass* easycurl_class;
  static JSObject* easycurl_proto;

  struct callback_data {
    ScriptContext* ctx;
    CURL* handle;
    JSContext* cx;
    JSObject* obj;
    jsrefcount ref;
    void* data;
    uint32_t dataSize;
    uint32_t dataMaxSize;
    uint32_t dataIndex;
  };

  /**
   * easycurl "slist" -- just a linked list of data
   *   only operations are "append".. and implied
   *   ctors/dtors.
   *
   */

  /**
   * returns a *copy* of slist as an array for debugging only
   */
  static JSBool easycurl_slist_toArray(JSContext *cx, uintN argc, jsval *vp) {

    // slist is a bit weird since it can have a NULL value for private data
    // so can't use GetInstancePrivate.  Have to check instance and then get
    // private data
    if (!JS_InstanceOf(cx, JS_THIS_OBJECT(cx, vp), easycurl_slist_class, NULL))
      return JS_FALSE;

    struct curl_slist *slist = (struct curl_slist *) (JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)));

    JSObject* ary = JS_NewArrayObject(cx, 0, NULL);
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(ary));
    int count = 0;
    while (slist != NULL) {
      JSString* s = JS_NewStringCopyN(cx, slist->data, strlen(slist->data));
      if (!s)
        return JS_FALSE;
      jsval v = STRING_TO_JSVAL(s);
      JSBool res = JS_SetElement(cx, ary, count, &v);
      if(!res) {
        return JS_FALSE;
      }
      slist = slist->next;
      ++count;
    }
    return JS_TRUE;
  }

  static JSBool easycurl_slist_append(JSContext *cx, uintN argc, jsval *vp) {
    struct curl_slist *slist = NULL;

    // slist is a bit weird since it can have a NULL value for private data
    // so can't use GetInstancePrivate.  Have to check instance and then get
    // private data
    if (!JS_InstanceOf(cx, JS_THIS_OBJECT(cx, vp), easycurl_slist_class, NULL))
      return JS_FALSE;

    slist = (struct curl_slist *) (JS_GetPrivate(cx, JS_THIS_OBJECT(cx, vp)));

    JSString* str = JS_ValueToString(cx, JS_ARGV(cx, vp) [0]);
    if (!str)
      return JS_FALSE;

    char* s = JS_EncodeString(cx, str);
    slist = curl_slist_append(slist, s);
    if (!slist) {
      JS_ReportOutOfMemory(cx);
      return JS_FALSE;
    }
    JS_free(cx, s);

    JS_SetPrivate(cx, JS_THIS_OBJECT(cx, vp), (void*) slist);
    return JS_TRUE;
  }

  // just makes an empty slist
  static JSBool easycurl_slist_ctor(JSContext *cx, uintN argc, jsval *vp) {
    JSObject* newobj = JS_NewObject(cx, easycurl_slist_class, NULL, NULL);
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(newobj));
    JS_SetPrivate(cx, newobj, (void*) 0);
    return JS_TRUE;
  }

  static void easycurl_slist_finalize(JSContext* cx, JSObject* obj) {
    struct curl_slist* list = (struct curl_slist*) (JS_GetPrivate(cx, obj));
    // SHOULD ASSERT HERE
    if (list)
      curl_slist_free_all(list);
  }

  /**
   * CALLBACKS
   *   C callbacks are defined which then call curl object javascript methods
   *
   */
  static size_t write_callback(void *ptr, size_t size, size_t nmemb, void *stream) {
    jsval rval;
    jsval argv[1];
    size_t len = size * nmemb;
    size_t copyLen = len;

    // pass back from the read/write/header operations
    struct callback_data* cb = (struct callback_data*) (stream);

    // get private data from handle for context
    JSContext* cx = cb->cx;
    JSObject* obj = cb->obj;

    JS_ResumeRequest(cx, cb->ref);
    JS_BeginRequest(cx);

    // copy data to the final result blob up to dataMaxSize
    if (cb->dataIndex + copyLen > cb->dataMaxSize) {
      copyLen = cb->dataMaxSize - cb->dataIndex;
    }

    if (cb->data == NULL){
      cb->data = JS_malloc(cx, copyLen);
      cb->dataSize += copyLen;
    } else if (cb->dataIndex + copyLen > cb->dataSize) {
      cb->data = JS_realloc(cx, cb->data, cb->dataSize + copyLen);
      cb->dataSize += copyLen;
    }
    if (copyLen > 0) {
      memcpy((char *) cb->data + cb->dataIndex, ptr, copyLen);
      cb->dataIndex += copyLen;
    } else {
      JS_ReportWarning(cx, "curl write callback: max payload size exceeded, drop incoming data");
    }

    // pass every chunk to the callback handler
    JSString* cData = JS_NewStringCopyN(cx, (const char *) ptr, len);
    argv[0] = STRING_TO_JSVAL(cData);
    JS_CallFunctionName(cx, obj, "write", 1, argv, &rval);

    JS_EndRequest(cx);
    cb->ref = JS_SuspendRequest(cx);

    return len;
  }

  static size_t header_callback(void *ptr, size_t size, size_t nmemb, void *stream) {
    jsval rval;
    jsval argv[1];
    size_t len = size * nmemb;

    // passback from the read/write/header operations
    struct callback_data* cb = (struct callback_data*) (stream);

    // get private data from handle for context
    JSContext* cx = cb->cx;
    JSObject* obj = cb->obj;

    JS_ResumeRequest(cx, cb->ref);
    JS_BeginRequest(cx);

    JSString* s = JS_NewStringCopyN(cx, (const char*) ptr, len);
    if (!s) {
      JS_EndRequest(cx);
      cb->ref = JS_SuspendRequest(cx);
      return -1;
    }

    argv[0] = STRING_TO_JSVAL(s);
    JS_CallFunctionName(cx, obj, "header", 1, argv, &rval);

    JS_EndRequest(cx);
    cb->ref = JS_SuspendRequest(cx);

    return len;
  }

  static int debug_callback(CURL *c, curl_infotype itype, char * buf, size_t len, void *userdata) {
    jsval rval;
    jsval argv[2];

    // passback from the read/write/header operations
    struct callback_data* cb = (struct callback_data*) (userdata);

    // get private data from handle for context
    JSContext* cx = cb->cx;
    JSObject* obj = cb->obj;

    JS_ResumeRequest(cx, cb->ref);
    JS_BeginRequest(cx);

    JSString* s = JS_NewStringCopyN(cx, buf, len);
    if (!s) {
      JS_EndRequest(cx);
      cb->ref = JS_SuspendRequest(cx);
      return -1;
    }

    argv[0] = INT_TO_JSVAL(itype);
    argv[1] = STRING_TO_JSVAL(s);
    JS_CallFunctionName(cx, obj, "debug", 2, argv, &rval);

    JS_EndRequest(cx);
    cb->ref = JS_SuspendRequest(cx);
    return 0;
  }

  /*******************************************************/
  /**
   * Helper function to determine what type of value curl_getinfo returns
   *
   */
  static int info_expected_type(int opt) {
    // opt & 0xf00000
    opt = (opt & CURLINFO_TYPEMASK);

    // opt is 0x100000 to 0x400000
    //  5*4 = 20;
    opt = opt >> 20;
    if (opt < 1 || opt > 4)
      opt = 0;
    return opt;
  }

  static JSBool jscurl_getinfo(JSContext *cx, uintN argc, jsval *vp) {
    ScriptContext *ctx = static_cast<ScriptContext *> (JS_GetContextPrivate(cx));
    JSRequest req(ctx);

    struct callback_data* cb = (struct callback_data*)
        (JS_GetInstancePrivate(cx, JS_THIS_OBJECT(cx, vp), easycurl_class, JS_ARGV(cx, vp)));

    if (!cb)
      return JS_FALSE;
    CURL* handle = cb->handle;
    int opt = 0;
    CURLcode c = CURLE_OK;

    jsval arg = JS_ARGV(cx, vp) [0];
    if (!JS_ValueToInt32(cx, arg, &opt))
      return JS_FALSE;

    // TODO: DONNY NOTES: can curl_easy_getinfo() fail for other reasons?
    //  does it have a way of reporting what went wrong?
    switch (info_expected_type(opt)) {
    // STRINGS
    case 1: {
      char* val = NULL;
      JSString* s = NULL;

      c = curl_easy_getinfo(handle, (CURLINFO) opt, &val);

      if (c != CURLE_OK) {
        JS_ReportError(cx, "Unable to get string value");
        return JS_FALSE;
      }
      s = JS_NewStringCopyN(cx, val, strlen(val));
      if (!s)
        return JS_FALSE;
      JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(s));
      return JS_TRUE;
    }
      // LONGS
    case 2: {
      long val = 0;
      c = curl_easy_getinfo(handle, (CURLINFO) opt, &val);
      if (c != CURLE_OK) {
        JS_ReportError(cx, "Unable to get long value");
        return JS_FALSE;
      }
      jsval rval;
      if (JS_NewNumberValue(cx, val, &rval)) {
        JS_SET_RVAL(cx, vp, rval);
        return JS_TRUE;
      }
      return JS_FALSE;
    }
      // DOUBLE
    case 3: {
      double dval = 0;
      c = curl_easy_getinfo(handle, (CURLINFO) opt, &dval);
      if (c != CURLE_OK) {
        JS_ReportError(cx, "Unable to get double value");
        return JS_FALSE;
      }
      jsval rval;
      if (JS_NewNumberValue(cx, dval, &rval)) {
        JS_SET_RVAL(cx, vp, rval);
        return JS_TRUE;
      }
      return JS_FALSE;
    }

      // SLIST OR OBJECTS
    case 4: {
      if (opt == CURLINFO_PRIVATE) {
        JS_ReportError(cx, "Sorry, that option isn't supported.");
        return JS_FALSE;
      }

      struct curl_slist *list = NULL;
      JSObject* newobj = NULL;

      c = curl_easy_getinfo(handle, (CURLINFO) opt, &list);
      if (c != CURLE_OK) {
        JS_ReportError(cx, "Unable to get slist value");
        return JS_FALSE;
      }

      newobj = JS_NewObject(cx, easycurl_slist_class, NULL, NULL);
      JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(newobj));
      JS_SetPrivate(cx, newobj, (void*) list);
      return JS_TRUE;
    }
    default:
      JS_ReportError(cx, "Unknown curl_easy_getinfo option");
      return JS_FALSE;
    }
  }

  /**
   * Helper function to determine what type of value a curl_setopt and
   * takes
   *
   * There are defined in decimal so we use "<" not bit ops
   *
   * @return 0 if invalid, 1,2,3 if valid
   */
  static int option_expected_type(int opt) {
    if (opt < CURLOPTTYPE_LONG)
      return 0; // invalid
    if (opt < CURLOPTTYPE_OBJECTPOINT)
      return 1; // long
    if (opt < CURLOPTTYPE_FUNCTIONPOINT)
      return 2; // string or slist
    if (opt < CURLOPTTYPE_OFF_T)
      return 3; // callback functions
    return 0; // invalid
  }

  static JSBool jscurl_setopt(JSContext *cx, uintN argc, jsval *vp) {
    ScriptContext *ctx = static_cast<ScriptContext *> (JS_GetContextPrivate(cx));
    JSRequest req(ctx);

    struct callback_data* cb = (struct callback_data*)
        (JS_GetInstancePrivate(cx, JS_THIS_OBJECT(cx, vp), easycurl_class, NULL));
    if (!cb)
      return JS_FALSE;

    CURL* handle = cb->handle;
    int opt = 0;
    CURLcode c = CURLE_OK;
    jsval arg = JS_ARGV(cx, vp) [0];
    if (!JS_ValueToInt32(cx, arg, &opt))
      return JS_FALSE;

    int optype = option_expected_type(opt);
    if (!optype) {
      JS_ReportError(cx, "Unknown or invalid curl_setopt option value");
      return JS_FALSE;
    }

    arg = JS_ARGV(cx, vp) [1];

    if (optype == 1) {
      int val;
      if (!JS_ValueToInt32(cx, arg, &val))
        return JS_FALSE;
      c = curl_easy_setopt(handle, (CURLoption) opt, val);
    } else if (optype == 2) {
      switch (opt) {

      case CURLOPT_HTTPHEADER:
      case CURLOPT_HTTP200ALIASES:
      case CURLOPT_QUOTE:
      case CURLOPT_POSTQUOTE:
      case CURLOPT_PREQUOTE:
      case CURLOPT_TELNETOPTIONS: {
        // take linked list
        if (JSVAL_IS_NULL(arg)) {
          // null is ok, used to reset to default behavior
          c = curl_easy_setopt(handle, (CURLoption) opt, NULL);
        } else {
          struct curl_slist* slist = NULL;
          JSObject* slistobj = JSVAL_TO_OBJECT(arg);

          if (!JS_InstanceOf(cx, slistobj, easycurl_slist_class, NULL))
            return JS_FALSE;
          slist = (struct curl_slist*) (JS_GetPrivate(cx, slistobj));
          // null is ok!
          c = curl_easy_setopt(handle, (CURLoption) opt, slist);
        }
      }
      break;

      case CURLOPT_PROTOCOLS: {
        JS_ReportError(cx, "Invalid curl_setopt option value: cannot modify supported protocols");
        return JS_FALSE;
      }
      break;

      default: {
        // DEFAULT CASE IS NORMAL STRING
        char* val = NULL;
        // TODO: why null and void?
        if (!JSVAL_IS_NULL(arg) && !JSVAL_IS_VOID(arg)) {
          JSString* str = JS_ValueToString(cx, arg);
          if (!str)
            return JS_FALSE;

          // TODO CHECK that arg is a function.. if not, it will abort anyways when curl calls it.
          val = JS_EncodeString(cx, str);
          c = curl_easy_setopt(handle, (CURLoption) opt, val);
          JS_free(cx, val);
        }
      }
      break;

      }
    }

    if (c == 0)
      return JS_TRUE;

    // something bad happened
    JS_ReportError(cx, "Failed with %d: %s", c, curl_easy_strerror(c));
    return JS_FALSE;
  }

  class SessionAttachedAsyncCurlObject : public ScriptContextAttachedObject {
  public:
    SessionAttachedAsyncCurlObject(ScriptContext* _pContext)
    : ScriptContextAttachedObject(_pContext),
      m_pRunAsUser(NULL)
    {
      if (Security::getCurrentlyLoggedInUser() != NULL) {
        m_pRunAsUser = new User(*Security::getCurrentlyLoggedInUser());
      }
    }

    virtual ~SessionAttachedAsyncCurlObject() {
      if (m_pRunAsUser) {
        delete m_pRunAsUser;
      }
    }

    void asyncCurlRequest(struct callback_data* cb, JSObject* _obj, jsval _function, ScriptFunctionRooter* _rooter) {
      CURLcode c;
      JSBool success;

      if (m_pRunAsUser != NULL) {
        Security* pSecurity = getContext()->getEnvironment().getSecurity();
        if (pSecurity != NULL) {
          pSecurity->signIn(m_pRunAsUser);
        }
      }

      {
        ScriptLock lock(cb->ctx);
        JSContextThread thread(cb->cx);
        JSRequest req(cb->ctx);
        cb->ref = JS_SuspendRequest(cb->cx);

        c = curl_easy_perform(cb->handle);

        JS_ResumeRequest(cb->cx, cb->ref);
      }

      if (!getIsStopped()) {
        ScriptLock lock(getContext());
        {
          JSContextThread req(getContext());
          ScriptObject sobj(cb->obj, *getContext());
          ScriptFunctionParameterList params(*getContext());

          if (c != CURLE_OK) {
              Logger::getInstance()->log(std::string("JavaScript: curl error ")+
                      curl_easy_strerror(c), lsError);
              success = JS_FALSE;
          } else {
              success = JS_TRUE;
          }
          params.addJSVal(BOOLEAN_TO_JSVAL(success));

          try {
            sobj.callFunctionByName<void>("asyncdone", params);
          } catch(ScriptException& e) {
            Logger::getInstance()->log("JavaScript: error calling curl callback handler: '" +
                std::string(e.what()) + "'", lsError);
          }

          delete _rooter;
        }

      } else {
        ScriptLock lock(getContext());
        JSContextThread req(getContext());
        delete _rooter;
      }

      delete this;
    }
  private:
      User* m_pRunAsUser;
  };

  static JSBool jscurl_perform(JSContext *cx, uintN argc, jsval *vp) {
    ScriptContext *ctx = static_cast<ScriptContext *> (JS_GetContextPrivate(cx));
    JSRequest req(ctx);

    struct callback_data* cb = (struct callback_data*)
        (JS_GetInstancePrivate(cx, JS_THIS_OBJECT(cx, vp), easycurl_class, JS_ARGV(cx, vp)));
    if (!cb)
      return JS_FALSE;
    cb->dataIndex = 0;

    cb->ref = JS_SuspendRequest(cx);
    CURLcode c = curl_easy_perform(cb->handle);
    JS_ResumeRequest(cx, cb->ref);

    if(JS_IsExceptionPending(cx)) {
      JS_ReportPendingException(cx);
      jsval exval;
      if(JS_GetPendingException(cx, &exval)) {
        JS_ClearPendingException(cx);
        JSString* errstr = JS_ValueToString(cx, exval);
        if(errstr != NULL) {
          char* errmsgBytes = JS_EncodeString(cx, errstr);
          std::string errMsg(errmsgBytes);
          JS_free(cx, errmsgBytes);
          Logger::getInstance()->log("JS Curl Exception: " + errMsg, lsWarning);
        }
      }
    }

    if (c == CURLE_OK) {
      JSString* s = JS_NewStringCopyN(cx, (const char *) cb->data, cb->dataIndex);
      JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(s));
      return JS_TRUE;
    }

    // something bad happened
    JS_ReportError(cx, "curl perform failed: %s [%d]", curl_easy_strerror(c), c);
    return JS_FALSE;
  }

  static JSBool jscurl_perform_async(JSContext *cx, uintN argc, jsval *vp) {
    ScriptContext *ctx = static_cast<ScriptContext *> (JS_GetContextPrivate(cx));

    struct callback_data* cb = (struct callback_data*)
        (JS_GetInstancePrivate(cx, JS_THIS_OBJECT(cx, vp), easycurl_class, JS_ARGV(cx, vp)));
    if (!cb)
      return JS_FALSE;
    cb->dataIndex = 0;

    JSObject* jsFunction;
    jsval functionVal;
    if (!JS_ConvertArguments(cx, argc, JS_ARGV(cx, vp), "o", &jsFunction)) {
      JS_ReportError(cx, "curl.perform_async(): invalid parameter");
      return JS_FALSE;
    }

    if (jsFunction == NULL || !JS_ObjectIsCallable(cx, jsFunction)) {
      JS_ReportError(cx, "curl.perform_async(): callback handler is not a function");
      return JS_FALSE;
    }
    functionVal = OBJECT_TO_JSVAL(jsFunction);

    boost::shared_ptr<ScriptObject> scriptObj(new ScriptObject(JS_THIS_OBJECT(cx, vp), *ctx));
    ScriptFunctionRooter* functionRoot(new ScriptFunctionRooter(ctx, scriptObj->getJSObject(), functionVal));

    SessionAttachedAsyncCurlObject* pAsyncCurlObj = new SessionAttachedAsyncCurlObject(ctx);
    boost::thread(boost::bind(&SessionAttachedAsyncCurlObject::asyncCurlRequest, pAsyncCurlObj,
      cb, JS_THIS_OBJECT(cx, vp), functionVal, functionRoot));

    JS_SET_RVAL(cx, vp, JSVAL_TRUE);
    return JS_TRUE;
  }

  /*
   * Internal Helper function
   */
  static JSBool jscurl_setupcallbacks(struct callback_data* cb) {
    CURL* handle = cb->handle;
    CURLcode c;

    c = curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_callback);
    if (c != CURLE_OK) {
      JS_ReportWarning(cb->cx, "curl setopt CURLOPT_WRITEFUNCTION failed: %s [d]", curl_easy_strerror(c), c);
    }
    c = curl_easy_setopt(handle, CURLOPT_WRITEDATA, cb);
    if (c != CURLE_OK) {
      JS_ReportWarning(cb->cx, "curl setopt CURLOPT_WRITEDATA failed: %s [d]", curl_easy_strerror(c), c);
    }

    c = curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, header_callback);
    if (c != CURLE_OK) {
      JS_ReportWarning(cb->cx, "curl setopt CURLOPT_HEADERFUNCTION failed: %s [d]", curl_easy_strerror(c), c);
    }
    c = curl_easy_setopt(handle, CURLOPT_HEADERDATA, cb);
    if (c != CURLE_OK) {
      JS_ReportWarning(cb->cx, "curl setopt CURLOPT_HEADERDATA failed: %s [d]", curl_easy_strerror(c), c);
    }

    c = curl_easy_setopt(handle, CURLOPT_READFUNCTION, write_callback);
    if (c != CURLE_OK) {
      JS_ReportWarning(cb->cx, "curl setopt CURLOPT_READFUNCTION failed: %s [d]", curl_easy_strerror(c), c);
    }
    c = curl_easy_setopt(handle, CURLOPT_READDATA, cb);
    if (c != CURLE_OK) {
      JS_ReportWarning(cb->cx, "curl setopt CURLOPT_READDATA failed: %s [d]", curl_easy_strerror(c), c);
    }

    c = curl_easy_setopt(handle, CURLOPT_DEBUGFUNCTION, debug_callback);
    if (c != CURLE_OK) {
      JS_ReportWarning(cb->cx, "curl setopt CURLOPT_DEBUGFUNCTION failed: %s [d]", curl_easy_strerror(c), c);
    }
    c = curl_easy_setopt(handle, CURLOPT_DEBUGDATA, cb);
    if (c != CURLE_OK) {
      JS_ReportWarning(cb->cx, "curl setopt CURLOPT_DEBUGDATA failed: %s [d]", curl_easy_strerror(c), c);
    }

    return JS_TRUE;
  }

  static JSBool jscurl_version(JSContext *cx, uintN argc, jsval *vp) {
    ScriptContext *ctx = static_cast<ScriptContext *> (JS_GetContextPrivate(cx));
    JSRequest req(ctx);

    char* v = curl_version();
    JSString* s = JS_NewStringCopyN(cx, v, strlen(v));
    if (!s)
      return JS_FALSE;
    JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(s));
    return JS_TRUE;
  }

  static JSBool jscurl_getdate(JSContext *cx, uintN argc, jsval *vp) {
    ScriptContext *ctx = static_cast<ScriptContext *> (JS_GetContextPrivate(cx));
    JSRequest req(ctx);

    JSString* str = JS_ValueToString(cx, JS_ARGV(cx, vp) [0]);
    if (!str)
      return JS_FALSE;

    char *val= JS_EncodeString(cx, str);
    double d = curl_getdate(val, NULL);
    JS_free(cx, val);

    jsval rval;
    JSBool success = JS_NewNumberValue(cx, d, &rval);
    JS_SET_RVAL(cx, vp, rval);

    return success;
  }

  static void easycurl_finalize(JSContext* cx, JSObject* obj) {
    struct callback_data* cb = static_cast<callback_data*> (JS_GetPrivate(cx, obj));
    if (cb) {
      /* On death, curl sends a "closing connection" debug message
       * If we are in GC, we don't want to send a request back to JS
       * (can occur only if verbose is 1)
       */
      curl_easy_setopt(cb->handle, CURLOPT_VERBOSE, 0);
      curl_easy_cleanup(cb->handle);

      if (cb->data) {
        JS_free(cx, cb->data);
      }
      JS_free(cx, cb);
      JS_SetPrivate(cx, obj, NULL);
    }
  }

  static JSBool easycurl_ctor(JSContext *cx, uintN argc, jsval *vp) {
    ScriptContext *ctx = static_cast<ScriptContext *>(JS_GetContextPrivate(cx));
    JSRequest req(ctx);

    struct callback_data* cb = NULL;
    JSObject* newobj = NULL;

    CURL* handle = curl_easy_init();
    if (!handle) {
      JS_ReportError(cx, "Unable to initialize libcurl!");
      return JS_FALSE;
    }
    newobj = JS_NewObject(cx, easycurl_class, NULL, NULL);
    if (!newobj)
      return JS_FALSE;
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(newobj));

    cb = (struct callback_data*) JS_malloc(cx, sizeof(struct callback_data));
    if (!cb) {
      JS_ReportOutOfMemory(cx);
      return JS_FALSE;
    }

    cb->ctx = ctx;
    cb->handle = handle;
    cb->cx = cx;
    cb->obj = newobj;
    cb->data = NULL;
    cb->dataIndex = cb->dataSize = 0;
    cb->dataMaxSize = 512 * 1024;

    PropertyNodePtr pPtr = DSS::getInstance()->getPropertySystem().getProperty("/config/spidermonkey/curlmaxmem");
    if (pPtr) {
      cb->dataMaxSize = pPtr->getIntegerValue();
    }

    if (!jscurl_setupcallbacks(cb)) {
      JS_free(cx, cb);
      JS_ReportError(cx, "failed to set libcurl callbacks");
      return JS_FALSE;
    }

    /* limit the available protocols to avoid security and resource issues */
    curl_easy_setopt(handle, CURLOPT_PROTOCOLS,
        CURLPROTO_HTTP |
        CURLPROTO_HTTPS |
        CURLPROTO_IMAP |
        CURLPROTO_IMAPS |
        CURLPROTO_POP3 |
        CURLPROTO_POP3S
        );

    /* avoid generating SIGALRM's */
    curl_easy_setopt(handle, CURLOPT_NOSIGNAL, 1);
    /* limit connect time */
    curl_easy_setopt(handle, CURLOPT_CONNECTTIMEOUT, 60);
    /* bail-out if there was silence for too long */
    curl_easy_setopt(handle, CURLOPT_LOW_SPEED_LIMIT, 1);
    curl_easy_setopt(handle, CURLOPT_LOW_SPEED_TIME, 30);
    /* limit the total duration of an operation */
    curl_easy_setopt(handle, CURLOPT_TIMEOUT, 600);

    JS_SetPrivate(cx, newobj, (void*) cb);
    return JS_TRUE;
  }

  /***********************************************************/

  static JSFunctionSpec easycurl_slist_fn[] = {
      JS_FS("append", easycurl_slist_append, 1, 0),
      JS_FS("toArray", easycurl_slist_toArray, 0, 0),
      JS_FS_END
  };

  static JSClass slist_class = {
      "easylist",
      JSCLASS_HAS_PRIVATE, // | JSCLASS_CONSTRUCT_PROTOTYPE,
      JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
      JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub,
      easycurl_slist_finalize, /* FINALIZE */
      JSCLASS_NO_OPTIONAL_MEMBERS
  };

  static JSFunctionSpec easycurl_fn[] = {
      JS_FS("setopt", jscurl_setopt, 2, 0),
      JS_FS("getinfo", jscurl_getinfo, 1, 0),
      JS_FS("perform", jscurl_perform, 0, 0),
      JS_FS("perform_async", jscurl_perform_async, 0, 0),
      JS_FS("getdate", jscurl_getdate, 1, 0),
      JS_FS("version", jscurl_version, 0, 0),
      JS_FS_END
  };

  static JSClass easy_class = {
      "easycurl",
      JSCLASS_HAS_PRIVATE, // | JSCLASS_CONSTRUCT_PROTOTYPE,
      JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
      JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, easycurl_finalize, /* FINALIZE */
      JSCLASS_NO_OPTIONAL_MEMBERS
  };

  CurlScriptContextExtension::CurlScriptContextExtension()
  : ScriptExtension(CurlScriptExtensionName)
  { } // ctor

  void CurlScriptContextExtension::extendContext(ScriptContext& _context) {
    easycurl_class = &easy_class;
    easycurl_proto = JS_InitClass(
        _context.getJSContext(),
        _context.getRootObject().getJSObject(),
        NULL, /* prototype   */
        easycurl_class, /* class       */
        easycurl_ctor, 0, /* ctor, args  */
        NULL, /* properties  */
        easycurl_fn, /* functions   */
        NULL, /* static_ps   */
        NULL /* static_fs   */
    );
    if (!easycurl_proto)
      return;

    if (!JS_DefineConstDoubles(_context.getJSContext(), easycurl_proto, easycurl_options))
      return;

    if (!JS_DefineConstDoubles(_context.getJSContext(), easycurl_proto, easycurl_info))
      return;

    easycurl_slist_class = &slist_class;
    easycurl_slist_proto = JS_InitClass(
        _context.getJSContext(),
        _context.getRootObject().getJSObject(),
        NULL, /* prototype   */
        easycurl_slist_class, /* class       */
        easycurl_slist_ctor, 0, /* ctor, args  */
        NULL, /* properties  */
        easycurl_slist_fn, NULL, /* static_ps   */
        NULL /* static_fs   */
    );
    if (!easycurl_slist_proto)
      return;
    return;
  }

}
