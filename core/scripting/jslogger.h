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

#ifndef __JS_LOGGER_H__
#define __JS_LOGGER_H__

#include "core/mutex.h"
#include "core/jshandler.h"

#include "core/event.h"
#include "core/eventinterpreterplugins.h"
#include "core/internaleventrelaytarget.h"

#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/ptr_container/ptr_map.hpp>

namespace dss {

  class ScriptLoggerExtension;

  class ScriptLogger {
  public:
    ScriptLogger(const std::string& _filePath, 
                 const std::string& _logname, 
                 ScriptLoggerExtension* _pExtension);
    ~ScriptLogger();
    void log(const std::string& text);
    void logln(const std::string& text);
    const std::string& getLogName() const { return m_logName; }
    void reopenLogfile();

  private:
    std::string m_logName;
    FILE *m_f;
    Mutex m_LogWriteMutex;
    ScriptLoggerExtension* m_pExtension;
    std::string m_fileName;
  };

  class ScriptLoggerExtension : public ScriptExtension {
  public:
    ScriptLoggerExtension(const std::string _directory, EventInterpreter& _eventInterpreter);
    virtual ~ScriptLoggerExtension();
    virtual void extendContext(ScriptContext& _context);
    boost::shared_ptr<ScriptLogger> getLogger(const std::string& _filename);
    void removeLogger(const std::string& _filename);

    int getNumberOfLoggers() const { return m_Loggers.size(); }

    void reopenLogfiles(Event& _event, const EventSubscription& _subscription);

  private:
    boost::ptr_map<const std::string, boost::weak_ptr<ScriptLogger> > m_Loggers;
    Mutex m_MapMutex;
    const std::string m_Directory;
    boost::shared_ptr<InternalEventRelayTarget> m_pRelayTarget;
    EventInterpreter& m_EventInterpreter;
  };
}

#endif

