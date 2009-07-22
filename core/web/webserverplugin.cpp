/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland
    Copyright (c) 2009 futureLAB AG, Winterthur, Switzerland

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

    Last change $Date: 2009-06-30 13:56:46 +0200 (Tue, 30 Jun 2009) $
    by $Author: pstaehlin $
*/


#include "webserverplugin.h"

namespace dss {

  WebServerPlugin::WebServerPlugin(const std::string& _uri, const std::string _file)
  : m_URI(_uri), m_File(_file)
  { } // ctor

  void WebServerPlugin::load() {
    if(!fileExists(m_File)) {
      throw runtime_error(string("Plugin '") + m_File + "' does not exist.");
    }
  } // load

  bool WebServerPlugin::handleRequest(const std::string& _uri, HashMapConstStringString& _parameter, string& result) {
    return false;
  } // handleRequest


} // namespace dss
