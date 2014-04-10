/*
 Copyright (c) 2013 aizo ag, Zurich, Switzerland

 Author: Christain Hitz <christian.hitz@aizo.com>

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

#ifndef __DSS_COMM_CHANNEL_H__
#define __DSS_COMM_CHANNEL_H__

#include <boost/thread/mutex.hpp>
#include <libcommchannel/commchannel.h>
#include <sys/types.h>
#include <list>
#include <vector>

#include "messages/messaging.pb.h"
#include "logger.h"

namespace dss {

class CommChannel : public CC::CommunicationChannelCallback
{
  __DECL_LOG_CHANNEL__
public:
    static CommChannel* createInstance();
    static CommChannel* getInstance();
    virtual ~CommChannel();

    virtual void messageReceived(boost::shared_ptr<CC::CommunicationData> comm);

    void run();
    bool isSceneLocked(uint32_t scene);
    bool requestLockedScenes();
    void suspendUpdateTask();
    void resumeUpdateTask();

    std::list<uint32_t> getLockedScenes();

private:
    CommChannel();
    void lockMessageList();
    void unlockMessageList();
    void shutdown();
    std::string sendMessage(const std::string& message);
    void sendWithType(dsmsg::Type msgtype);

    void taskThread();
    static void *staticThreadProc(void *arg);

private:
    static CommChannel* m_instance;
    int m_shutdown_flag;
    bool m_running;
    pthread_t m_thread;
    pthread_mutex_t m_mutex;
    boost::mutex m_scenes_mutex;
    pthread_cond_t m_condition;
    std::list<boost::shared_ptr<CC::CommunicationData> > m_datalist;
    std::list<uint32_t> m_locked_scenes;
    CC::CommunicationChannelServer *m_pServer;
};

}

#endif /* COMM_CHANNEL_H_ */
