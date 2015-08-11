/*
Copyright (c) 2010 digitalSTROM.org, Zurich, Switzerland

Authors: Sergey 'Jin' Bostandzhyan <jin@dev.digitalstrom.org>,
         Michael Tross, aizo GmbH <michael.tross@aizo.com>,
         Patrick Staehlin <pstaehlin@futurelab.ch>

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

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif


#include "jslogger.h"
#include "scriptobject.h"
#include "src/dss.h"
#include "src/logger.h"
#include "src/internaleventrelaytarget.h"
#include "src/propertysystem.h"
#include "src/datetools.h"
#include "src/stringconverter.h"

#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>

#define LOG_OBJECT_IDENTIFIER   "logfile"

namespace dss {

  const std::string ScriptLoggerExtensionName = "scriptloggerextension";
  const std::string LoggerObjectName = "ScriptLoggerContextWrapper";

  class ScriptLoggerContextWrapper {
  public:
    ScriptLoggerContextWrapper(boost::shared_ptr<ScriptLogger> _logger)
    : m_ScriptLogger(_logger)
    { }

    boost::shared_ptr<ScriptLogger> getLogger() { return m_ScriptLogger; }
  private:
    boost::shared_ptr<ScriptLogger> m_ScriptLogger;
  }; // ScriptLoggerContextWrapper

  JSBool ScriptLoggerExtension_log_common(JSContext *cx, uintN argc, jsval *vp, bool newline) {
    ScriptContext *ctx = static_cast<ScriptContext *>(JS_GetContextPrivate(cx));
    JSRequest req(ctx);
    ScriptLoggerExtension *ext = dynamic_cast<ScriptLoggerExtension*>(ctx->getEnvironment().getExtension(ScriptLoggerExtensionName));
    assert(ext != NULL);

    if (!JS_ConvertArguments(cx, argc, JS_ARGV(cx, vp), "*"))
      return JS_FALSE;

    std::stringstream sstream;
    for (uintN i = 0; i < argc; i++)
    {
      JSString *unicode_str;
      char *src;

      unicode_str = JS_ValueToString(cx, (JS_ARGV(cx, vp))[i]);
      if (unicode_str == NULL)
        return JS_FALSE;

      src = JS_EncodeString(cx, unicode_str);
      sstream << std::string(src);
      JS_free(cx, src);
    }

    JSObject* obj = JS_THIS_OBJECT(cx, vp);
    ScriptLoggerContextWrapper* wrapper = static_cast<ScriptLoggerContextWrapper*> (JS_GetPrivate(cx, obj));

    if(wrapper != NULL) {
      Logger::getInstance()->log(sstream.str());
      if(newline) {
        wrapper->getLogger()->logln(sstream.str());
      } else {
        wrapper->getLogger()->log(sstream.str());
      }
    } else {
      Logger::getInstance()->log("ScriptLoggerExtension_log_common: wrapper is null!", lsFatal);
      return JS_FALSE;
    }
    return JS_TRUE;
  }

  JSBool ScriptLoggerExtension_log(JSContext *cx, uintN argc, jsval *vp) {
    return ScriptLoggerExtension_log_common(cx, argc, vp, false);
  }

  JSBool ScriptLoggerExtension_logln(JSContext *cx, uintN argc, jsval *vp) {
    return ScriptLoggerExtension_log_common(cx, argc, vp, true);
  }

  JSBool ScriptLogger_construct(JSContext *cx, uintN argc, jsval *vp);
  void ScriptLogger_finalize(JSContext *cx, JSObject *obj);

  static JSClass ScriptLogger_class = {
    "Logger", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStandardClasses,
    JS_ResolveStub,
    JS_ConvertStub,
    ScriptLogger_finalize,
    JSCLASS_NO_OPTIONAL_MEMBERS
  };

  static JSFunctionSpec ScriptLogger_methods[] = {
    JS_FS("log", ScriptLoggerExtension_log, 1, 0),
    JS_FS("logln", ScriptLoggerExtension_logln, 1, 0),
    JS_FS_END
  };

  JSBool ScriptLogger_construct(JSContext *cx, uintN argc, jsval *vp) {
    ScriptContext *ctx = static_cast<ScriptContext *>(JS_GetContextPrivate(cx));
    JSRequest req(ctx);
    ScriptLoggerExtension *ext = dynamic_cast<ScriptLoggerExtension*>(ctx->getEnvironment().getExtension(ScriptLoggerExtensionName));
    assert(ext != NULL);

    JSString *jsFileName = JS_ValueToString(cx, argc ? JS_ARGV(cx, vp)[0] : JSVAL_VOID);
    if (!jsFileName)
      return false;

    char *filename = JS_EncodeString(cx, jsFileName);
    std::string logFileName(filename);
    JS_free(cx, filename);

    try {
      if (!endsWith(logFileName, ".log")) {
        Logger::getInstance()->log("Log file name must have .log extension", lsError);
        return JS_FALSE;
      }

      boost::shared_ptr<ScriptLogger> pLogger = ext->getLogger(logFileName);
      ScriptLoggerContextWrapper* wrapper = new ScriptLoggerContextWrapper(pLogger);

      JSObject *obj = JS_NewObject(cx, &ScriptLogger_class, NULL, NULL);
      JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(obj));
      JS_SetPrivate(cx, obj, wrapper);

      return JS_TRUE;
    } catch(const ScriptException& e) {
      Logger::getInstance()->log(std::string("ScriptLogger: Caught script exception: ") + e.what(), lsError);
    }

    return JS_FALSE;
  }

  void ScriptLogger_finalize(JSContext *cx, JSObject *obj) {
    ScriptLoggerContextWrapper* pWrapper = static_cast<ScriptLoggerContextWrapper*>(JS_GetPrivate(cx, obj));
    if(pWrapper != NULL) {
      JS_SetPrivate(cx, obj, NULL);
      delete pWrapper;
    }
  } // finalize_set

  ScriptLogger::ScriptLogger(const std::string& _filePath, 
                             const std::string& _filename, 
                             ScriptLoggerExtension* _pExtension) {
    ///\todo check log name for validity, prevent attempts to switch
    /// directories, etc.
    StringConverter st("UTF-8", "UTF-8");
    m_pExtension = _pExtension;
    m_logName = st.convert(_filename);
    m_fileName = st.convert(_filePath + _filename);
#ifdef __USE_GNU
    m_f = fopen(m_fileName.c_str(), "a+e");
#else
    m_f = fopen(m_fileName.c_str(), "a+");
    if (m_f) {
      fcntl(fileno(m_f), F_SETFD, FD_CLOEXEC);
    }
#endif
    if (!m_f) {
      throw std::runtime_error("Could not open file " + m_fileName + " for writing");
    }
    if(DSS::hasInstance()) {
      DSS::getInstance()->getPropertySystem().setStringValue("/system/js/logfiles/" + _filename, m_fileName, true, false);
      DSS::getInstance()->getPropertySystem().setStringValue("/config/subsystems/WebServer/files/" + _filename, m_fileName, true, false);
    }
  } // ctor

  void ScriptLogger::log(const std::string& text) {
    boost::mutex::scoped_lock lock(m_LogWriteMutex);
    if (m_f) {
      DateTime timestamp = DateTime();

      std::string out = "[" + timestamp.toISO8601_ms_local() + "] " + text;
      size_t written = fwrite(out.c_str(), 1, out.size(), m_f);
      fflush(m_f);
      if (written < text.size()) {
        throw std::runtime_error("Could not complete write operation to log file");
      }
    }
  } // log

  void ScriptLogger::logln(const std::string& text) {
    log(text + "\n");
  } // logln

  void ScriptLogger::reopenLogfile() {
    boost::mutex::scoped_lock lock(m_LogWriteMutex);
    if (m_f) {
      fclose(m_f);
#ifdef __USE_GNU
      m_f = fopen(m_fileName.c_str(), "a+e");
#else
      m_f = fopen(m_fileName.c_str(), "a+");
      if (m_f) {
        fcntl(fileno(m_f), F_SETFD, FD_CLOEXEC);
      }
#endif
      if (!m_f) {
        throw std::runtime_error("Could not open file " + m_fileName + " for writing");
      }
    }
  }

  ScriptLogger::~ScriptLogger() {
    if(m_f) {
      fclose(m_f);
    }

    if (m_pExtension) {
      m_pExtension->removeLogger(m_logName);
    }
  } // dtor


  //================================================== ScriptLoggerExtension

  ScriptLoggerExtension::ScriptLoggerExtension(const std::string _directory, EventInterpreter& _eventInterpreter) 
  : ScriptExtension(ScriptLoggerExtensionName),
    m_Directory(addTrailingBackslash(_directory)), m_EventInterpreter(_eventInterpreter)
  {
    EventInterpreterInternalRelay* pRelay =
        dynamic_cast<EventInterpreterInternalRelay*>(m_EventInterpreter.getPluginByName(EventInterpreterInternalRelay::getPluginName()));
      m_pRelayTarget = boost::make_shared<InternalEventRelayTarget>(boost::ref<EventInterpreterInternalRelay>(*pRelay));

      if (m_pRelayTarget != NULL) {
        boost::shared_ptr<EventSubscription> signalSubscription(
                new dss::EventSubscription(
                    EventName::Signal,
                    EventInterpreterInternalRelay::getPluginName(),
                    m_EventInterpreter,
                    boost::shared_ptr<SubscriptionOptions>())
        );
        m_pRelayTarget->subscribeTo(signalSubscription);
        m_pRelayTarget->setCallback(boost::bind(&ScriptLoggerExtension::reopenLogfiles, this, _1, _2));
      }
  } // ctor

  ScriptLoggerExtension::~ScriptLoggerExtension() {
  } // dtor

  void ScriptLoggerExtension::reopenLogfiles(Event& _event, const EventSubscription& _subscription) {
    int signal = strToIntDef( _event.getPropertyByName("signum"), -1);
    if (signal == SIGUSR1) {
      boost::mutex::scoped_lock lock(m_MapMutex);
      std::map<const std::string, boost::weak_ptr<ScriptLogger> >::iterator i;
      for (i = m_Loggers.begin(); i != m_Loggers.end(); i++) {
        if (!i->second.expired()) {
          i->second.lock()->reopenLogfile();
        }
      }
    }
  }

  void ScriptLoggerExtension::extendContext(ScriptContext& _context) {
    JS_InitClass(
        _context.getJSContext(),
        _context.getRootObject().getJSObject(),
        NULL,
        &ScriptLogger_class,
        ScriptLogger_construct,
        1, NULL,
        ScriptLogger_methods,
        NULL, NULL);
  } // extendContext

  boost::shared_ptr<ScriptLogger> ScriptLoggerExtension::getLogger(const std::string& _filename) {
    boost::shared_ptr<ScriptLogger> result;
    boost::mutex::scoped_lock lock(m_MapMutex);
    std::map<const std::string, boost::weak_ptr<ScriptLogger> >::iterator i = m_Loggers.find(_filename);
    if(i == m_Loggers.end()) {
      result.reset(new ScriptLogger(m_Directory, _filename, this));
      boost::weak_ptr<ScriptLogger> loggerWeakPtr(result);
      m_Loggers[_filename] = loggerWeakPtr;
    } else {
      result = m_Loggers[_filename].lock();
    }
    return result;
  } // getLogger

  void ScriptLoggerExtension::removeLogger(const std::string& _filename) {
    boost::mutex::scoped_lock lock(m_MapMutex);
    std::map<const std::string, boost::weak_ptr<ScriptLogger> >::iterator i = m_Loggers.find(_filename);
    if(i != m_Loggers.end()) {
      m_Loggers.erase(i);
    } else {
      Logger::getInstance()->log("Tried to remove nonexistent logger '" + _filename + "' !", lsWarning);
    }
  } // removeLogger

} // namespace dss


