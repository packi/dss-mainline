#ifndef WEBSERVER_H_
#define WEBSERVER_H_

#include <shttpd/shttpd.h>

#include "base.h"
#include "thread.h"
#include "subsystem.h"

#include <string>

namespace dss {

  class IDeviceInterface;

  class WebServer : public Subsystem,
                    private Thread {
  private:
    struct shttpd_ctx* m_SHttpdContext;
  private:
    virtual void Execute();
  protected:
    bool IsDeviceInterfaceCall(const std::string& _method);
    string CallDeviceInterface(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg, IDeviceInterface* _interface);

    static void JSONHandler(struct shttpd_arg* _arg);
    string HandleApartmentCall(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg, bool& _handled);
    string HandleZoneCall(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg, bool& _handled);
    string HandleDeviceCall(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg, bool& _handled);
    string HandleSetCall(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg, bool& _handled);
    string HandlePropertyCall(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg, bool& _handled);
    string HandleEventCall(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg, bool& _handled);
    string HandleSimCall(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg, bool& _handled);
    string HandleDebugCall(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg, bool& _handled);

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
