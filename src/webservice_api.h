#ifndef __WEBSERVICE_API__
#define __WEBSERVICE_API__

#include <stdexcept>

#include "logger.h"
#include "http_client.h"
#include "webservice_connection.h" // URLRequestCallback

namespace dss {


// --------------------------------------------------------------------
// helper

/**
 * Did the user authorize cloud service
 * @return
 */
bool webservice_communication_authorized();

struct WebserviceReply {
  int code;
  std::string desc;
};

typedef enum {
  REST_OK = 0,
  NETWORK_ERROR,
  JSON_ERROR,
} RestTransferStatus_t;

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

class WebserviceCallDone {
public:
  /**
   * @RestTransferStatus -- connection established, valid json received
   * @WebserviceReply -- webserver reply
   */
  virtual void done(RestTransferStatus_t status, WebserviceReply reply) = 0;
};

typedef boost::shared_ptr<WebserviceCallDone> WebserviceCallDone_t;

class StatusReplyChecker : public URLRequestCallback {
  __DECL_LOG_CHANNEL__
public:
  /**
   * StatusReplyChecker - triggered by http client, will parse the returned
   * JSON as WebserviceReply structure, and trigger provided callbac
   * @callback: trigger
   */
  StatusReplyChecker(WebserviceCallDone_t callback) : m_callback(callback) {};
  virtual ~StatusReplyChecker() {};
  virtual void result(long code, const std::string &result);
private:
  WebserviceCallDone_t m_callback;
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

  /**
   * doModelChanged - schedules asynchronous http request
   * @type: what part of the model changed
   * @callback: provides network and server status
   */
  static void doModelChanged(ChangeType type, WebserviceCallDone_t callback);

  template <class iterator>
    static void doUploadSensorData(iterator begin, iterator end,
                                   WebserviceCallDone_t callback);

  static void doDssBackAgain(WebserviceCallDone_t callback);
};

class WebserviceAccessManagement {
  __DECL_LOG_CHANNEL__
public:
  /**
   * Asynchronously notifies the cloud about removed application token
   * if you want to retry the request in case of network failure you can pass
   * request_done callback argument
   * @token: token to be deleted
   * @callback: provides network and server status
   */
  static void doNotifyTokenDeleted(const std::string &token,
                                   WebserviceCallDone_t callback);
};

}

#endif
