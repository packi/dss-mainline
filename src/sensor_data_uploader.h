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
#include "webservice_api.h"

namespace dss {



  class SensorLog : public WebserviceCallDone,
                    public boost::enable_shared_from_this<SensorLog> {
    __DECL_LOG_CHANNEL__
    enum {
      max_post_events = 50,
      max_elements = 10000,
    };
  public:
    typedef std::vector<boost::shared_ptr<Event> >::iterator It;
    typedef void (*doUploadSensorDataFunction)(It begin, It end,
                                               WebserviceCallDone_t callback);

    SensorLog(const std::string hubName, doUploadSensorDataFunction doUpload)
      : m_pending_upload(false), m_hubName(hubName), m_doUpload(doUpload) {};
    virtual ~SensorLog() {};
    void append(boost::shared_ptr<Event> event, bool highPrio = false);
    void triggerUpload();
    void done(RestTransferStatus_t status, WebserviceReply reply);
  private:
    std::vector<boost::shared_ptr<Event> > m_events;
    std::vector<boost::shared_ptr<Event> > m_eventsHighPrio;
    std::vector<boost::shared_ptr<Event> > m_uploading;
    boost::mutex m_lock;
    bool m_pending_upload;
    const std::string m_hubName;
    doUploadSensorDataFunction m_doUpload;
  };

  class SensorDataUploadMsHubPlugin : public EventInterpreterPlugin,
                                      private PropertyListener {
  private:
    __DECL_LOG_CHANNEL__
    virtual void propertyChanged(PropertyNodePtr _caller,
                                 PropertyNodePtr _changedNode);
    void scheduleBatchUploader();

  public:
    SensorDataUploadMsHubPlugin(EventInterpreter* _pInterpreter);
    virtual ~SensorDataUploadMsHubPlugin();
    virtual void handleEvent(Event& _event, const EventSubscription& _subscription);
    virtual void subscribe();
  private:
    boost::shared_ptr<SensorLog> m_log;
    PropertyNodePtr websvcEnabledNode;
  };

  class SensorDataUploadDsHubPlugin : public EventInterpreterPlugin,
                                      private PropertyListener {
  private:
    __DECL_LOG_CHANNEL__
    virtual void propertyChanged(PropertyNodePtr _caller,
                                 PropertyNodePtr _changedNode);
    void scheduleBatchUploader();

  public:
    SensorDataUploadDsHubPlugin(EventInterpreter* _pInterpreter);
    virtual ~SensorDataUploadDsHubPlugin();
    virtual void handleEvent(Event& _event, const EventSubscription& _subscription);
    virtual void subscribe();
  private:
    boost::shared_ptr<SensorLog> m_log;
    PropertyNodePtr websvcEnabledNode;
  };
}
#endif
