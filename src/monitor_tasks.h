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

#ifndef __DSS_MONITOR_TASKS_H__
#define __DSS_MONITOR_TASKS_H__

#include "taskprocessor.h"
#include "event.h"
#include "model/apartment.h"

namespace dss {

class SensorMonitorTask : public Task {
public:
  SensorMonitorTask(Apartment* _apartment) : m_Apartment(_apartment) {};
  virtual ~SensorMonitorTask() {};
  virtual void run();
private:
  Apartment *m_Apartment;
  bool checkZoneValue(boost::shared_ptr<Group> _group, int _sensorType, DateTime _ts);
};

class HeatingMonitorTask : public Task {
public:
  HeatingMonitorTask(Apartment* _apartment, boost::shared_ptr<Event> _event) :
    m_Apartment(_apartment),
    m_event(_event)
    {};
  virtual ~HeatingMonitorTask() {};
  virtual void run();
private:
  Apartment *m_Apartment;
  boost::shared_ptr<Event> m_event;
  void syncZone(int _zoneID);
};

class HeatingValveProtectionTask : public Task {
public:
  HeatingValveProtectionTask(Apartment* _apartment, boost::shared_ptr<Event> _event) :
    m_Apartment(_apartment),
    m_event(_event)
    {};
  virtual ~HeatingValveProtectionTask() {};
  virtual void run();
private:
  Apartment *m_Apartment;
  boost::shared_ptr<Event> m_event;
  static int m_zoneIndex;
};

}// namespace

#endif//__DSS_MONITOR_TASKS_H__
