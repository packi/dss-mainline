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
    virtual void Execute();
  protected:
    bool IsDeviceInterfaceCall(const std::string& _method);
    string CallDeviceInterface(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg, IDeviceInterface* _interface, Session* _session);

    static void JSONHandler(struct shttpd_arg* _arg);
    string HandleApartmentCall(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg, bool& _handled, Session* _session);
    string HandleZoneCall(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg, bool& _handled, Session* _session);
    string HandleDeviceCall(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg, bool& _handled, Session* _session);
    string HandleSetCall(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg, bool& _handled, Session* _session);
    string HandlePropertyCall(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg, bool& _handled, Session* _session);
    string HandleEventCall(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg, bool& _handled, Session* _session);
    string HandleCircuitCall(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg, bool& _handled, Session* _session);
    string HandleSimCall(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg, bool& _handled, Session* _session);
    string HandleDebugCall(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg, bool& _handled, Session* _session);

    static void HTTPListOptions(struct shttpd_arg* _arg);
    static void EmitHTTPHeader(int _code, struct shttpd_arg* _arg, const std::string& _contentType = "text/html");

  protected:
    virtual void DoStart();
  public:
    WebServer(DSS* _pDSS);
    ~WebServer();

    virtual void Initialize();

  }; // WebServer

}

#endif /*WEBSERVER_H_*/
