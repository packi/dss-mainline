/*
Copyright (c) 2010 digitalSTROM.org, Zurich, Switzerland

Author: Sergey 'Jin' Bostandzhyan <jin@dev.digitalstrom.org>

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

#include "jslogger.h"
#include "scriptobject.h"
#include "core/dss.h"
#include "core/logger.h"
#include "core/propertysystem.h"

#include <stdio.h>

#define LOG_OBJECT_IDENTIFIER   "logfile"
namespace dss {
  const std::string ScriptLoggerExtensionName = "scriptloggerextension";

  const std::string LoggerObjectName = "ScriptLoggerContextWrapper";
  class ScriptLoggerContextWrapper : public ScriptContextAttachedObject {
  public:
    ScriptLoggerContextWrapper(ScriptContext* _pContext, boost::shared_ptr<ScriptLogger> _logger)
    : ScriptContextAttachedObject(_pContext, LoggerObjectName + _logger->getLogName()),
      m_ScriptLogger(_logger)
    { }

    boost::shared_ptr<ScriptLogger> getLogger() { return m_ScriptLogger; }
  private:
    boost::shared_ptr<ScriptLogger> m_ScriptLogger;
  }; // ScriptLoggerContextWrapper

  JSBool ScriptLoggerExtension_log_common(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval, bool newline) {
    JSString *str;
    ScriptContext *ctx = static_cast<ScriptContext *>(JS_GetContextPrivate(cx));
    ScriptLoggerExtension *ext = dynamic_cast<ScriptLoggerExtension*>(ctx->getEnvironment().getExtension(ScriptLoggerExtensionName));
    assert(ext != NULL);

    // ignore empty lines
    if(argc >=1) {
      try {
        if (!JSVAL_IS_STRING(argv[0])) {
          return JS_TRUE;
        }
        str = JS_ValueToString(cx, argv[0]);
        if (!str) {
          return JS_TRUE;
        }
      } catch(const ScriptException& e) {
        Logger::getInstance()->log(std::string("ScriptLogger: Caught script exception: ") + e.what());
      }
    } else {
      return JS_TRUE;
    }

    jsval v;
    if (JS_GetProperty(cx, obj, LOG_OBJECT_IDENTIFIER, &v) == JS_TRUE) {
      if(v != JSVAL_VOID) {
        JSString *logfile = JSVAL_TO_STRING(v);
        std::string logfileStr = JS_GetStringBytes(logfile);
        ScriptContextAttachedObject* attachedObj = ctx->getAttachedObjectByName(LoggerObjectName + logfileStr);
        ScriptLoggerContextWrapper* wrapper =
          dynamic_cast<ScriptLoggerContextWrapper*>(attachedObj);
        if(wrapper != NULL) {
          Logger::getInstance()->log(JS_GetStringBytes(str));
          if(newline) {
            wrapper->getLogger()->logln(JS_GetStringBytes(str));
          } else {
            wrapper->getLogger()->log(JS_GetStringBytes(str));
          }
        } else {
          Logger::getInstance()->log("Could not find logger named: " + LoggerObjectName + logfileStr, lsWarning);
        }
      }
    }
    return JS_TRUE;
  }

  JSBool ScriptLoggerExtension_log(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    return ScriptLoggerExtension_log_common(cx, obj, argc, argv, rval, false);
  }

  JSBool ScriptLoggerExtension_logln(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    return ScriptLoggerExtension_log_common(cx, obj, argc, argv, rval, true);
  }

  JSBool ScriptLogger_construct(JSContext *cx, JSObject *obj, uintN argc,
                                jsval *argv, jsval *rval) {
    JSString *str;
    ScriptContext *ctx = static_cast<ScriptContext *>(JS_GetContextPrivate(cx));
    ScriptLoggerExtension *ext = dynamic_cast<ScriptLoggerExtension*>(ctx->getEnvironment().getExtension(ScriptLoggerExtensionName));
    assert(ext != NULL);

    if(argc >=1) {
      try {
        if (!JSVAL_IS_STRING(argv[0])) {
          Logger::getInstance()->log("Expected log file name (string) as constructor parameter");
          return JS_FALSE;
        }
        str = JS_ValueToString(cx, argv[0]);
        if (!str) {
          Logger::getInstance()->log("Expected log file name (string) as constructor parameter");
          return JS_FALSE;
        }

        boost::shared_ptr<ScriptLogger> pLogger = ext->getLogger(JS_GetStringBytes(str));
        ctx->attachObject(new ScriptLoggerContextWrapper(ctx, pLogger));
        jsval v = STRING_TO_JSVAL(str);
        JS_SetProperty(cx, obj, LOG_OBJECT_IDENTIFIER, &v);
        JSBool foundp;
        JS_SetPropertyAttributes(cx, obj, LOG_OBJECT_IDENTIFIER, JSPROP_READONLY, &foundp);
        return JS_TRUE;

      } catch(const ScriptException& e) {
        Logger::getInstance()->log(std::string("ScriptLogger: Caught script exception: ") + e.what());
      }
    } else {
      Logger::getInstance()->log("Expected log file name (string) as constructor parameter");
    }

    return JS_FALSE;
  }

  static JSClass ScriptLogger_class = {
    "Logger", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,  JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStandardClasses,
    JS_ResolveStub,
    JS_ConvertStub,  JS_FinalizeStub, JSCLASS_NO_OPTIONAL_MEMBERS
  };

  static JSFunctionSpec ScriptLogger_methods[] = {
    {"log", ScriptLoggerExtension_log, 1, 0, 0},
    {"logln", ScriptLoggerExtension_logln, 1, 0, 0},
    {NULL, NULL, 0, 0, 0},
  };

  static JSFunctionSpec ScriptLogger_static_methods[] = {
    {"getChannel", ScriptLoggerExtension_log, 1, 0, 0},
    {NULL, NULL, 0, 0, 0},
  };


  ScriptLogger::ScriptLogger(const std::string& _filePath, 
                             const std::string& _filename, 
                             ScriptLoggerExtension* _pExtension) {
    ///\todo check log name for validity, prevent attempts to switch
    /// directories, etc.

    m_pExtension = _pExtension;
    m_logName = _filename;
    std::string fileName = _filePath + _filename;
    m_f = fopen(fileName.c_str(), "a+");
    if (!m_f) {
      throw std::runtime_error("Could not open file " + fileName + " for writing");
    }
    if(DSS::hasInstance()) {
      DSS::getInstance()->getPropertySystem().setStringValue("/system/js/logsfiles/" + _filename, fileName, true, false);
      DSS::getInstance()->getPropertySystem().setStringValue("/config/subsystems/WebServer/files/" + _filename, fileName, true, false);
    }
  } // ctor

  void ScriptLogger::log(const std::string& text) {
    if (m_f) {
      struct tm t;
      time_t now = time( NULL );
      localtime_r( &now, &t );
      std::string out = "[" + dateToISOString<std::string>(&t) + "] " + text;
      m_LogWriteMutex.lock();
      size_t written = fwrite(out.c_str(), 1, out.size(), m_f);
      fflush(m_f);
      m_LogWriteMutex.unlock();
      if (written < text.size()) {
        throw std::runtime_error("Could not complete write operation to log file");
      }
    }
  } // log

  void ScriptLogger::logln(const std::string& text) {
    log(text + "\n");
  } // logln

  ScriptLogger::~ScriptLogger() {
    Logger::getInstance()->log("Destroying logger with filename: " + m_logName);
    if(m_f) {
      fclose(m_f);
    }
    m_pExtension->removeLogger(m_logName);
  } // dtor

  
  //================================================== ScriptLoggerExtension

  ScriptLoggerExtension::ScriptLoggerExtension(const std::string _directory) 
  : ScriptExtension(ScriptLoggerExtensionName),
    m_Directory(addTrailingBackslash(_directory))
  { } // ctor

  ScriptLoggerExtension::~ScriptLoggerExtension() {
  } // dtor

  void ScriptLoggerExtension::extendContext(ScriptContext& _context) {
    JS_InitClass(_context.getJSContext(),
                 _context.getRootObject().getJSObject(),
                NULL, &ScriptLogger_class, ScriptLogger_construct, 1, NULL,
                ScriptLogger_methods, NULL, ScriptLogger_static_methods);

  } // extendContext

  boost::shared_ptr<ScriptLogger> ScriptLoggerExtension::getLogger(const std::string& _filename) {
    boost::shared_ptr<ScriptLogger> result;
    m_MapMutex.lock();
    boost::ptr_map<const std::string, boost::weak_ptr<ScriptLogger> >::iterator i = m_Loggers.find(_filename);
    if(i == m_Loggers.end()) {
      result.reset(new ScriptLogger(m_Directory, _filename, this));
      boost::weak_ptr<ScriptLogger> loggerWeakPtr(result);
      m_Loggers[_filename] = loggerWeakPtr;
    } else {
      result = m_Loggers[_filename].lock();
    }
    m_MapMutex.unlock();
    return result;
  } // getLogger

  void ScriptLoggerExtension::removeLogger(const std::string& _filename) {
    m_MapMutex.lock();
    boost::ptr_map<const std::string, boost::weak_ptr<ScriptLogger> >::iterator i = m_Loggers.find(_filename);
    if(i != m_Loggers.end()) {
      m_Loggers.erase(i);
    } else {
      Logger::getInstance()->log("Tried to remove nonexistent logger '" + _filename + "' !", lsWarning);
    }
    m_MapMutex.unlock();
  } // removeLogger

} // namespace dss


