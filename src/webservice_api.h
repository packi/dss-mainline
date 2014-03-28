#ifndef __WEBSERVICE_API__
#define __WEBSERVICE_API__

#include "config.h"
#ifdef HAVE_CURL // No HTTP client otherwise

#include <stdexcept>

#include "logger.h"
#include "url.h"
#include "webservice_connection.h" // URLRequestCallback

namespace dss {


// --------------------------------------------------------------------
// helper

struct WebserviceReply {
    int code;
    std::string desc;
};

class ParseError : public std::runtime_error {
public:
    ParseError(const std::string& _message);
};

/**
 * Extract return code and message
 * @param json -- json encoded reply
 * @return decodod struct or throw ParseError if failed
 */
WebserviceReply parse_reply(const char* json);

class StatusReplyChecker : public URLRequestCallback {
  __DECL_LOG_CHANNEL__
public:
  StatusReplyChecker() {};
  virtual ~StatusReplyChecker() {};
  virtual void result(long code, boost::shared_ptr<URLResult> result);
};


// --------------------------------------------------------------------
// interface calls

class WebserviceApartment {
  __DECL_LOG_CHANNEL__
public:
  typedef enum {
    ApartmentChange = 1,
    TimedEventChange = 2,
    UDAChange = 3,
  } ChangeType;

  static void doModelChangedNotification(ChangeType type);
};

}

#endif // No HTTP client otherwise

#endif
