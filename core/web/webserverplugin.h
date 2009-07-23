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

#ifndef WEBSERVERPLUGIN_H_
#define WEBSERVERPLUGIN_H_

#include "core/base.h"

#include <string>

namespace dss {

  class DSS;

  class WebServerPlugin {
  private:
    std::string m_URI;
    std::string m_File;
    void* m_Handle;
    typedef bool (*HandleRequest_t)(const std::string& _uri, HashMapConstStringString& _parameter, DSS& _dss, std::string& result);
    HandleRequest_t m_pHandleRequest;
  public:
    WebServerPlugin(const std::string& _uri, const std::string& _file);
    ~WebServerPlugin();
    void load();
    bool handleRequest(const std::string& _uri, HashMapConstStringString& _parameter, DSS& _dss, std::string& result);
  };

} // namespace dss

#endif /* WEBSERVERPLUGIN_H_ */
