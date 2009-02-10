#ifndef WEBSERVER_H_
#define WEBSERVER_H_

#include <shttpd/shttpd.h>
#include "thread.h"
#include "subsystem.h"

#include <string>

namespace dss {

  class WebServer : public Subsystem,
                    private Thread {
  private:
    struct shttpd_ctx* m_SHttpdContext;
  private:
    virtual void Execute();
  protected:
    static void JSONHandler(struct shttpd_arg* _arg);
    static void HTTPListOptions(struct shttpd_arg* _arg);
    static void EmitHTTPHeader(int _code, struct shttpd_arg* _arg, const std::string& _contentType = "text/html");
  public:
    WebServer(DSS* _pDSS);
    ~WebServer();

    virtual void Initialize();
    virtual void Start();

  }; // WebServer

}

#endif /*WEBSERVER_H_*/
