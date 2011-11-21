/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland

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

#ifndef DSSIMJS_H_
#define DSSIMJS_H_

#include <string>
#include <vector>

#include <boost/scoped_ptr.hpp>

#include "dssim.h"
#include "core/scripting/jshandler.h"

namespace dss {

  class DSIDJSCreator : public DSIDCreator {
  public:
    DSIDJSCreator(const std::vector<std::string>& _fileNames, const std::string& _pluginName, DSSim& _simulator);
    virtual DSIDInterface* createDSID(const dss_dsid_t _dsid, const devid_t _shortAddress, const DSMeterSim& _dsMeter);
  private:
    boost::scoped_ptr<ScriptEnvironment> m_pScriptEnvironment;
    std::vector<std::string> m_FileNames;
    DSSim& m_Simulator;
  }; // DSIDPluginCreator

}

#endif /* DSSIMJS_H_ */
