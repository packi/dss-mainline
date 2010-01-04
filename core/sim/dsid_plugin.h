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

#ifndef DSID_PLUGIN_H_INCLUDED
#define DSID_PLUGIN_H_INCLUDED

#include "dssim.h"

namespace dss {

  class DSIDPluginCreator : public DSIDCreator {
  private:
    void* m_SOHandle;
    int (*createInstance)();
  public:
    DSIDPluginCreator(void* _soHandle, const char* _pluginName);

    virtual DSIDInterface* createDSID(const dsid_t _dsid, const devid_t _shortAddress, const DSModulatorSim& _modulator);
  }; // DSIDPluginCreator

} // namespace dss

#endif /* DSID_PLUGIN_H_INCLUDED */
