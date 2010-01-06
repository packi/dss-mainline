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

#ifndef BUSSCANNER_H
#define BUSSCANNER_H

#include "core/logger.h"

namespace dss {

  class StructureQueryBusInterface;
  class Apartment;
  class DSMeter;
  class ModelMaintenance;
  
  class BusScanner {
  public:
    BusScanner(StructureQueryBusInterface& _interface, Apartment& _apartment, ModelMaintenance& _maintenance);
    bool scanDSMeter(DSMeter& _dsMeter);
  private:
    void log(const std::string& _line, aLogSeverity _severity = lsDebug);
  private:
    Apartment& m_Apartment;
    StructureQueryBusInterface& m_Interface;
    ModelMaintenance& m_Maintenance;
  };

} // namespace dss

#endif // BUSSCANNER_H
