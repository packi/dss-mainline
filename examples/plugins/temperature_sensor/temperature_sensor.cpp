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
#include "../../../core/sim/plugin/pluginbase.h"


class DSIDTemperatureSensor : public DSID {
private:
  int m_RemotePort;
  std::string m_RemoteHost;
public:
    DSIDTemperatureSensor() {
    }

    virtual ~DSIDTemperatureSensor() {}

    virtual void callScene(const int _sceneNr) {}
    virtual void saveScene(const int _sceneNr) {}
    virtual void undoScene(const int _sceneNr) {}

    virtual void increaseValue(const int _parameterNr = -1) {}
    virtual void decreaseValue(const int _parameterNr = -1) {}

    virtual void enable() {}
    virtual void disable() {}

    virtual void startDim(bool _directionUp, const int _parameterNr = -1) {}
    virtual void endDim(const int _parameterNr = -1) {}
    virtual void setValue(const double _value, int _parameterNr = -1) {}

    virtual double getValue(int _parameterNr = -1) const { return 0.0; }

    virtual void setConfigurationParameter(const std::string& _name, const std::string& _value) {
    }

    virtual unsigned char udiSend(unsigned char _value, bool _lastByte) {
      return _value;
    }

    virtual int getFunctionID() {
      return 0xbeef;
    }
};

class TemperatureSensorDSIDFactory : public DSIDFactory {
protected:
  TemperatureSensorDSIDFactory()
  : DSIDFactory("example.temperature_sensor")
  { }

  virtual ~TemperatureSensorDSIDFactory() {}

  virtual DSID* doCreateDSID() {
    return new DSIDTemperatureSensor();
  }

public:
  static void create() {
    if(m_Instance == NULL) {
      m_Instance = new TemperatureSensorDSIDFactory();
    }
  }

}; // VLCRemoteDSIDFactory

static void _init(void) __attribute__ ((constructor));
static void _init(void) {
  TemperatureSensorDSIDFactory::create();
} // init

