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

#ifndef DS485SCRIPTEXTENSION_H
#define DS485SCRIPTEXTENSION_H

#include <vector>

#include <boost/shared_ptr.hpp>

#include "core/jshandler.h"
#include "core/ds485client.h"
#include "core/mutex.h"

namespace dss {

  class ScriptObject;
  class DS485CommandFrame;
  class DS485ScriptExtension;
  class FrameSenderInterface;
  class FrameBucketHolder;
  class FrameBucketCollector;

  class DS485ScriptExtension : public ScriptExtension {
  public:
    DS485ScriptExtension(FrameSenderInterface& _frameSender, FrameBucketHolder& _busInterfaceHandler);
    virtual ~DS485ScriptExtension() {}

    virtual void extendContext(ScriptContext& _context);
    void sendFrame(ScriptObject& _frame);
    void sendFrame(ScriptObject& _frame, boost::shared_ptr<ScriptObject> _callbackObject, jsval _function, int _timeout);
    void setCallback(int _sourceID, int _functionID, boost::shared_ptr<ScriptObject> _callbackObject, jsval _function, int _timeout);

    void removeCallback(boost::shared_ptr<FrameBucketCollector> _pCallback);
  private:
    boost::shared_ptr<DS485CommandFrame> frameFromScriptObject(ScriptObject& _object);
  private:
    boost::shared_ptr<DS485Client> m_pClient;
    FrameSenderInterface& m_FrameSender;
    FrameBucketHolder& m_FrameBucketHolder;
    Mutex m_CallbackMutex;
    typedef std::vector<boost::shared_ptr<FrameBucketCollector> > CallbackVector;
    CallbackVector m_Callbacks;
  }; // PropertyScriptExtension

} // namespace dss

#endif // DS485SCRIPTEXTENSION_H
