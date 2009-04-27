#include <stdlib.h>

#include <iostream>
#include <map>

#include <pthread.h>

#include <Poco/Net/StreamSocket.h>
#include <Poco/Net/SocketStream.h>
#include <Poco/Net/SocketAddress.h>
#include <Poco/StreamCopier.h>
#include <Poco/Exception.h>

#include "../../../core/sim/include/dsid_plugin.h"
#include "../../../core/ds485const.h"
#include "../../../core/base.h"
#include "../../../core/sim/plugin/pluginmedia.h"


class DSIDVLCRemote : public DSIDMedia {
private:
  int m_RemotePort;
  std::string m_RemoteHost;
public:
    DSIDVLCRemote() {
      m_RemotePort = 4212;
      m_RemoteHost = "127.0.0.1";
    }
    virtual ~DSIDVLCRemote() {}

    virtual void sendCommand(const std::string& _command) {
      try {
               Poco::Net::SocketAddress sa(m_RemoteHost, m_RemotePort);
               Poco::Net::StreamSocket sock(sa);
               Poco::Net::SocketStream str(sock);

               std::cout << "before sending: " << _command << std::endl;
               str << _command << "\r\n" << std::flush;
               str << "logout\r\n" << std::flush;
               std::cout << "done sending" << std::endl;

               sock.close();
       } catch (Poco::Exception& exc) {
               std::cerr << "******** [vlc_remote.so] exception caught: " << exc.displayText() << std::endl;
       }
    } // sendCommand

    virtual void powerOn() {
      sendCommand("pause");
    } // powerOn

    virtual void powerOff() {
      sendCommand("pause");
    } // powerOff

    virtual void deepOff() {
      sendCommand("stop");
    } // deepOff

    virtual void nextSong() {
      if(lastWasOff()) {
        powerOn();
      }
      sendCommand("next");
    } // nextSont
    virtual void previousSong() {
      if(lastWasOff()) {
        powerOn();
      }
      sendCommand("prev");
    } // previousSont

    virtual void volumeUp() {
      sendCommand("volup");
    } // volumeUp

    virtual void volumeDown() {
      sendCommand("voldown");
    } // volumeDown

    virtual void SetConfigurationParameter(const std::string& _name, const std::string& _value) {
      if(_name == "port") {
        m_RemotePort = dss::StrToInt(_value);
      } else if(_name == "host") {
        m_RemoteHost = _value;
      }
    }
};

class VLCRemoteDSIDFactory : public DSIDFactory {
protected:
  VLCRemoteDSIDFactory()
  : DSIDFactory("example.vlc_remote")
  { }

  virtual ~VLCRemoteDSIDFactory() {}

  virtual DSID* DoCreateDSID() {
    return new DSIDVLCRemote();
  }

public:
  static void Create() {
    if(m_Instance == NULL) {
      m_Instance = new VLCRemoteDSIDFactory();
    }
  }

}; // VLCRemoteDSIDFactory

static void _init(void) __attribute__ ((constructor));
static void _init(void) {
  VLCRemoteDSIDFactory::Create();
} // init

