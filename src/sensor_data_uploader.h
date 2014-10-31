/*
 * Copyright (c) 2014 digitalSTROM.org, Zurich, Switzerland
 *
 * Authors: Andreas Fenkart, digitalSTROM ag <andreas.fenkart@dev.digitalstrom.org>
 *
 * This file is part of digitalSTROM Server.
 *
 * digitalSTROM Server is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * digitalSTROM Server is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with digitalSTROM Server. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef __SENSOR_UPLOADER_H__
#define __SENSOR_UPLOADER_H__

#include "event.h"
#include "logger.h"

namespace dss {

  class SensorLog;

  class SensorDataUploadPlugin : public EventInterpreterPlugin,
                                 private PropertyListener {
  private:
    __DECL_LOG_CHANNEL__
    virtual void propertyChanged(PropertyNodePtr _caller,
                                 PropertyNodePtr _changedNode);
    void scheduleBatchUploader();

  public:
    SensorDataUploadPlugin(EventInterpreter* _pInterpreter);
    virtual ~SensorDataUploadPlugin();
    virtual void handleEvent(Event& _event, const EventSubscription& _subscription);
    virtual void subscribe();
  private:
    boost::shared_ptr<SensorLog> m_log;
    PropertyNodePtr websvcEnabledNode;
  };
}
#endif
