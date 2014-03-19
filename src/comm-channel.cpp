/*
 Copyright (c) 2013 aizo ag, Zurich, Switzerland

 Authors: Christain Hitz <christian.hitz@aizo.com>
          Sergey Bostandzhyan <jin@dev.digitalstrom.org>

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

#ifdef HAVE_CONFIG_H
    #include "config.h"
#endif

#include "comm-channel.h"
#include <pthread.h>
#include <libcommchannel/commchannel.h>
#include <libcommchannel/exceptions.h>
#include "messages/device-scene-table-update.pb.h"

#include "src/logger.h"
#include "src/dss.h"
#include "src/propertysystem.h"

namespace dss {

CommChannel* CommChannel::m_instance = NULL;

CommChannel* CommChannel::createInstance()
{
    if (m_instance == NULL)
    {
        m_instance = new CommChannel();
        if (m_instance == NULL)
        {
            throw std::runtime_error("could not create CommChannel class");
        }
    }

    return m_instance;
}

CommChannel* CommChannel::getInstance()
{
    if (m_instance == NULL)
    {
        throw std::runtime_error("CommChannel not initialized");
    }

    return m_instance;
}

CommChannel::CommChannel()
        : m_shutdown_flag(false),
          m_running(false),
          m_thread(0)
{
    int ret;

    ret = pthread_mutex_init(&m_mutex, NULL);
    if (ret != 0)
    {
        throw std::runtime_error("failed to initialize message list mutex: " +
                std::string(strerror(errno)));
    }

    ret = pthread_cond_init(&m_condition, NULL);
    if (ret != 0)
    {
        pthread_mutex_destroy(&m_mutex);
        throw std::runtime_error("failed to initialize message handling "
                                 "wakeup condition: " +
                                  std::string(strerror(errno)));
    }

    DSS::getInstance()->getPropertySystem().setIntValue(
            "/config/communication/receiveport", 50112, true, false);

    DSS::getInstance()->getPropertySystem().setIntValue(
            "/config/communication/sendport", 50111, true, false);

    int dSSPort = DSS::getInstance()->getPropertySystem().getIntValue(
            "/config/communication/receiveport");
    m_pServer = new CC::CommunicationChannelServer(dSSPort);
    if (!m_pServer)
    {
        pthread_mutex_destroy(&m_mutex);
        pthread_cond_destroy(&m_condition);
        throw std::runtime_error("failed to initialize CommunicationChannelServer");
    }

    m_pServer->addCallback(this);
}

void CommChannel::shutdown()
{
    m_shutdown_flag = true;

    lockMessageList();
    pthread_cond_signal(&m_condition);
    unlockMessageList();
    if (m_thread)
    {
        pthread_join(m_thread, NULL);
        m_thread = 0;
    }
}

CommChannel::~CommChannel()
{
    shutdown();

    if (m_pServer)
    {
        m_pServer->removeCallback(this);
        delete m_pServer;
    }

    m_instance = NULL;

    pthread_mutex_destroy(&m_mutex);
    pthread_cond_destroy(&m_condition);
}

void CommChannel::lockMessageList()
{
    int ret = pthread_mutex_lock(&m_mutex);
    if (ret != 0)
    {
        throw std::runtime_error("Failed to lock mutex: " +
                std::string(strerror(errno)));
    }
}

void CommChannel::unlockMessageList()
{
    int ret = pthread_mutex_unlock(&m_mutex);
    if (ret != 0)
    {
        throw std::runtime_error("Failed to unlock mutex: " +
                std::string(strerror(errno)));
    }
}

void CommChannel::messageReceived(boost::shared_ptr<CC::CommunicationData> comm)
{
    lockMessageList();
    m_datalist.push_back(comm);
    pthread_cond_signal(&m_condition);
    unlockMessageList();
}

void CommChannel::run()
{
    m_running = true;

    int ret = pthread_create(&m_thread, NULL, CommChannel::staticThreadProc, this);
    if (ret != 0)
    {
        m_running = false;
        throw std::runtime_error("Could not start task thread");
    }
}

bool CommChannel::isSceneLocked(uint32_t scene)
{
    boost::mutex::scoped_lock lock(m_scenes_mutex);
    std::list<uint32_t>::iterator it;
    for (it = m_locked_scenes.begin(); it != m_locked_scenes.end(); it++)
    {
        if (*it == scene)
        {
            return true;
        }
    }

    return false;
}

std::list<uint32_t> CommChannel::getLockedScenes()
{
    std::list<uint32_t> locked_scenes;
    boost::mutex::scoped_lock lock(m_scenes_mutex);
    // return a copy for thread safety's sake
    locked_scenes.assign(m_locked_scenes.begin(), m_locked_scenes.end());
    return locked_scenes;
}

void CommChannel::suspendUpdateTask()
{
    sendWithType(dsmsg::DEVICE_SCENE_TABLE_UPDATE_SUSPEND);
}

void CommChannel::resumeUpdateTask()
{
    sendWithType(dsmsg::DEVICE_SCENE_TABLE_UPDATE_RESUME);
}

void CommChannel::sendWithType(dsmsg::Type msgtype)
{
    dsmsg::Message message;
    message.set_type(msgtype);
    std::string response = sendMessage(message.SerializeAsString());
    if (!response.empty())
    {
        message.Clear();
        message.ParseFromString(response);
        if (message.generic_res().result() != 0)
        {
            Logger::getInstance()->log("[CommChannel] received response: " + 
                    message.generic_res().description(), lsWarning);
            return;
        }
    }
}

bool CommChannel::requestLockedScenes()
{
    dsmsg::Message message;
    message.set_type(dsmsg::DEVICE_SCENE_TABLE_UPDATE_STATE_REQUEST);
    std::string response = sendMessage(message.SerializeAsString());
    if (!response.empty())
    {
        message.Clear();
        message.ParseFromString(response);
        if (message.generic_res().result() != 0)
        {
            Logger::getInstance()->log("[CommChannel] received response: " + 
                    message.generic_res().description(), lsWarning);
            return false;
        }

        if (message.type() != dsmsg::DEVICE_SCENE_TABLE_UPDATE_STATE_RESPONSE)
        {
            Logger::getInstance()->log("[CommChannel] received unexpected "
                    "response!", lsError);
            return false;
        }

        boost::mutex::scoped_lock lock(m_scenes_mutex);
        m_locked_scenes.clear();
        if (message.HasExtension(dsmsg::device_scene_table_update_state))
        {
            dsmsg::DeviceSceneTableUpdateState sceneList = message.GetExtension(
                        dsmsg::device_scene_table_update_state);
            for (int i = 0; i < sceneList.locked_scene_size(); i++)
            {
                m_locked_scenes.push_back(sceneList.locked_scene(i));
            }
        }

        return true;
    }

    return false;
}

std::string CommChannel::sendMessage(const std::string& message)
{
    int dSAPort = DSS::getInstance()->getPropertySystem().getIntValue("/config/communication/sendport");
    CC::CommunicationChannelClient cc(dSAPort);

    try
    {
        boost::shared_ptr<CC::CommunicationData> c = cc.sendData((unsigned char*) message.c_str(), message.length());
        const char *data = (const char *) c->getDataPtr();
        if (data != NULL)
        {
            return std::string(data);
        }
        else
        {
            Logger::getInstance()->log("[CommChannel] sending message failed: "
                                       "no response", lsError);
        }
    }
    catch (CC::Exception& ex)
    {
        Logger::getInstance()->log("[CommChannel] sending message failed: " +
                                   ex.getMessage());
    }
    return std::string();
}

void *CommChannel::staticThreadProc(void *arg)
{
    CommChannel *instance = (CommChannel *) arg;
    instance->taskThread();
    pthread_exit(NULL);
    return NULL;
}

void CommChannel::taskThread()
{
    try
    {
        lockMessageList();
        while (!m_shutdown_flag)
        {
            if (m_datalist.empty())
            {
                pthread_cond_wait(&m_condition, &m_mutex);
                continue;
            }

            boost::shared_ptr<CC::CommunicationData> c = m_datalist.front();
            m_datalist.pop_front();

            unlockMessageList();

            const unsigned char *data = c->getDataPtr();
            if (data != NULL)
            {
                dsmsg::Message response;
                response.set_type(dsmsg::GENERIC_RESPONSE);
                dsmsg::ResultCode result = dsmsg::RESULT_OK;
                std::string resultDescription = "OK";

                dsmsg::Message received_message;
                std::string dataString((const char *) data);
                received_message.ParseFromString(dataString);
                switch (received_message.type())
                {
                    case dsmsg::DEVICE_SCENE_TABLE_UPDATE_STATE:
                    {
                        boost::mutex::scoped_lock lock(m_scenes_mutex);
                        m_locked_scenes.clear();
                        if (received_message.HasExtension(
                                    dsmsg::device_scene_table_update_state))
                        {
                            dsmsg::DeviceSceneTableUpdateState sceneList =
                                received_message.GetExtension(
                                        dsmsg::device_scene_table_update_state);
                            for (int i = 0; i < sceneList.locked_scene_size(); i++)
                            {
                                m_locked_scenes.push_back(
                                        sceneList.locked_scene(i));
                            }
                        }
                        break;
                    }
                    default:
                    {
                        result = dsmsg::RESULT_MESSAGE_UNKNOWN;
                        resultDescription = "Message unknown";
                        break;
                    }
                }

                if (response.type() == dsmsg::GENERIC_RESPONSE)
                {
                    dsmsg::GenericResponse *resp = response.mutable_generic_res();
                    resp->set_result(result);
                    resp->set_description(resultDescription);
                }
                std::string responseString = response.SerializeAsString();

                try
                {
                    m_pServer->sendReply(c->getID(), (unsigned char *) responseString.c_str(), responseString.length());
                }
                catch (CC::Exception &ex)
                {
                    Logger::getInstance()->log(
                                      "[CommChannel] Could not send reply:" +
                                      ex.getMessage());
                }
            }

            if (!m_shutdown_flag) {
                lockMessageList();
            }
        }
    }
    catch (CC::Exception &ex)
    {
        Logger::getInstance()->log("[CommChannel] exit communication loop: " +
                                   ex.getMessage());
    }
    catch (std::runtime_error &er)
    {
        Logger::getInstance()->log("[CommChannel] exit communication loop: " +
                                   std::string(er.what()));
    }
    catch (...)
    {
        Logger::getInstance()->log("[CommChannel] exit communication loop");
    }
    unlockMessageList();
    return;
}

}  // namespace dss
