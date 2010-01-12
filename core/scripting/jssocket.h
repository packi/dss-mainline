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

#ifndef JSSOCKET_H_
#define JSSOCKET_H_

#include <vector>

#include <boost/shared_ptr.hpp>

#include "core/jshandler.h"

namespace dss {

  class SocketHelper;

  class SocketScriptContextExtension : public ScriptExtension {
  public:
    SocketScriptContextExtension();

    virtual void extendContext(ScriptContext& _context);
    void removeSocketHelper(boost::shared_ptr<SocketHelper> _helper);
    void addSocketHelper(boost::shared_ptr<SocketHelper> _helper);

//    JSObject* createJSSocket(ScriptContext& _ctx);
  private:
    Mutex m_SocketHelperMutex;
    typedef std::vector<boost::shared_ptr<SocketHelper> > SocketHelperVector;
    SocketHelperVector m_SocketHelper;
  }; // SocketScriptExtension

} // namespace dss

#endif /* JSSOCKET_H_ */
