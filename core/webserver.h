#ifndef WEBSERVER_H_
#define WEBSERVER_H_

#include <shttpd/shttpd.h>
#include "thread.h"
#include "configuration.h"

namespace dss {

  class WebServer : public Thread {
  private:
    struct shttpd_ctx* m_SHttpdContext;
  protected:
    static void JSONHandler(struct shttpd_arg* _arg);
    static void HTTPListOptions(struct shttpd_arg* _arg);
    static void EmitHTTPHeader(int _code, struct shttpd_arg* _arg, const string _contentType = "text/html");
  public:
    WebServer();
    ~WebServer();

    void Initialize(Config& _config);

    virtual void Execute();
  }; // WebServer

}

#endif /*WEBSERVER_H_*/
