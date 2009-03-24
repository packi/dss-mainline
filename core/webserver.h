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
    void CallDeviceInterface(const std::string& _method, IDeviceInterface* _interface, int _param);

    static void JSONHandler(struct shttpd_arg* _arg);
    void HandleApartmentCall(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg);
    void HandleZoneCall(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg);
    void HandleDeviceCall(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg);
    void HandleSetCall(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg);
    void HandlePropertyCall(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg);
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
