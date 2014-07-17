/*
    Copyright (c) 2014 digitalSTROM AG, Zurich, Switzerland

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

#ifndef __DSS_SENSOR_MONITOR_H__
#define __DSS_SENSOR_MONITOR_H__

#include "taskprocessor.h"
#include "model/apartment.h"

namespace dss {

class SensorMonitorTask : public Task {
public:
  SensorMonitorTask(Apartment* _apartment) : m_Apartment(_apartment) {};
  virtual ~SensorMonitorTask() {};
  virtual void run();
private:
  Apartment *m_Apartment;
};

}// namespace

#endif//__DSS_SENSOR_MONITOR_H__
