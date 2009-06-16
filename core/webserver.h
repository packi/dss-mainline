#ifndef WEBSERVER_H_
#define WEBSERVER_H_

#include <shttpd/shttpd.h>

#include "base.h"
#include "thread.h"
#include "subsystem.h"
#include "session.h"

#include <string>

#include <boost/ptr_container/ptr_map.hpp>

namespace dss {


  class IDeviceInterface;

  typedef boost::ptr_map<const int, Session> SessionByID;

  class WebServer : public Subsystem,
                    private Thread {
  private:
    struct shttpd_ctx* m_SHttpdContext;
    int m_LastSessionID;
    SessionByID m_Sessions;
  private:
    virtual void execute();
  protected:
    bool isDeviceInterfaceCall(const std::string& _method);
    string callDeviceInterface(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg, IDeviceInterface* _interface, Session* _session);

    static void jsonHandler(struct shttpd_arg* _arg);
    string handleApartmentCall(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg, bool& _handled, Session* _session);
    string handleZoneCall(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg, bool& _handled, Session* _session);
    string handleDeviceCall(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg, bool& _handled, Session* _session);
    string handleSetCall(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg, bool& _handled, Session* _session);
    string handlePropertyCall(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg, bool& _handled, Session* _session);
    string handleEventCall(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg, bool& _handled, Session* _session);
    string handleCircuitCall(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg, bool& _handled, Session* _session);
    string handleStructureCall(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg, bool& _handled, Session* _session);
    string handleSimCall(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg, bool& _handled, Session* _session);
    string handleDebugCall(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg, bool& _handled, Session* _session);

    static void httpBrowseProperties(struct shttpd_arg* _arg);
    static void emitHTTPHeader(int _code, struct shttpd_arg* _arg, const std::string& _contentType = "text/html");

  protected:
    virtual void doStart();
  public:
    WebServer(DSS* _pDSS);
    ~WebServer();

    virtual void initialize();

  }; // WebServer

}

#endif /*WEBSERVER_H_*/
