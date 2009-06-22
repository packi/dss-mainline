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

*/

#include <stdlib.h>

#include <iostream>
#include <map>

#include <pthread.h>

#include <Poco/Net/StreamSocket.h>
#include <Poco/Net/SocketStream.h>
#include <Poco/Net/SocketAddress.h>
#include <Poco/StreamCopier.h>
#include <Poco/Exception.h>

#include "../../core/sim/include/dsid_plugin.h"
#include "../../core/sim/plugin/pluginmedia.h"
#include "../../core/ds485const.h"
#include "../../core/base.h"

class DSIDSlimSlaveRemote : public DSIDMedia {
private:
  int m_RemotePort;
  int m_DefaultVolume;
  std::string m_RemoteHost;
  std::string m_PlayerMACHeader;
public:
    DSIDSlimSlaveRemote() {
      m_RemotePort = 4212;
      m_RemoteHost = "127.0.0.1";
      m_PlayerMACHeader = "";
      m_DefaultVolume = -1;
    }
    virtual ~DSIDSlimSlaveRemote() {}

    virtual void sendCommand(const std::string& _command) {
      try {
        if(m_PlayerMACHeader.empty()) {
         std::cerr << "Parameter playerMAC not specified. NOT sending command " << _command << endl;
        }
        Poco::Net::SocketAddress sa(m_RemoteHost, m_RemotePort);
        Poco::Net::StreamSocket sock(sa);
        Poco::Net::SocketStream str(sock);

        std::cout << "before sending: " << _command << std::endl;
        str << m_PlayerMACHeader << " ";
        str << _command << "\n" << std::flush;
        std::cout << m_PlayerMACHeader << " " << _command << std::endl;
        std::cout << "done sending" << std::endl;

        sock.close();
       } catch (Poco::Exception& exc) {
               std::cerr << "******** [slim_slave.so] exception caught: " << exc.displayText() << std::endl;
       }
    }

    virtual void powerOn() {
      sendCommand("power 1");
      if(m_DefaultVolume != -1) {
        sendCommand("mixer volume " + dss::intToString(m_DefaultVolume));
      }
      sendCommand("play");
    }

    virtual void powerOff() {
      sendCommand("power 0");
    }

    virtual void deepOff() {
      sendCommand("power 0");
    }

    virtual void nextSong() {
      if(lastWasOff()) {
        powerOn();
      }
      sendCommand("playlist index +1");
    }

    void previousSong() {
      if(lastWasOff()) {
        powerOn();
      }
      sendCommand("playlist index -1");
    }

    virtual void volumeUp() {
      sendCommand("mixer volume +5");
    }

    virtual void volumeDown() {
      sendCommand("mixer volume -6\r\nmixer volume +1");
    }

    virtual void setConfigurationParameter(const std::string& _name, const std::string& _value) {
      if(_name == "port") {
        m_RemotePort = dss::strToInt(_value);
      } else if(_name == "host") {
        m_RemoteHost = _value;
      } else if(_name == "playermac") {
        std::cout << "config-param: " << _value << std::endl;
        m_PlayerMACHeader = _value;
        dss::replaceAll(m_PlayerMACHeader, ":", "%3A");
        std::cout << "after: " << m_PlayerMACHeader << std::endl;
      } else if(_name == "volume") {
        m_DefaultVolume = dss::strToInt(_value);
      }
    }
};

class SlimSlaveDSIDFactory : public DSIDFactory {
protected:
  SlimSlaveDSIDFactory()
  : DSIDFactory("aizo.slim_slave")
  { }

  virtual ~SlimSlaveDSIDFactory() {}

  virtual DSID* doCreateDSID() {
    return new DSIDSlimSlaveRemote();
  }

public:
  static void create() {
    if(m_Instance == NULL) {
      m_Instance = new SlimSlaveDSIDFactory();
    }
  }

}; // SlimSlaveDSIDFactory

static void _init(void) __attribute__ ((constructor));
static void _init(void) {
  SlimSlaveDSIDFactory::create();
} // _init

