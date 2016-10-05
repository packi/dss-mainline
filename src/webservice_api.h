#ifndef __WEBSERVICE_API__
#define __WEBSERVICE_API__

#include <stdexcept>

#include "logger.h"
#include "http_client.h"
#include "webservice_connection.h" // URLRequestCallback

namespace dss {

class JSONWriter;

// --------------------------------------------------------------------
// helper

namespace MsHub {
  std::vector<std::string> uploadEvents(void);
  void toJson(const boost::shared_ptr<Event> &event, JSONWriter& json);

  enum {
    OK = 0,
    RC_NOT_ENABLED = 7,
  };
}

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

class WebserviceCallDone {
public:
  /**
   * @RestTransferStatus -- connection established, valid json received
   * @WebserviceReply -- webserver reply
   */
  virtual void done(RestTransferStatus_t status, WebserviceReply reply) = 0;
};

typedef boost::shared_ptr<WebserviceCallDone> WebserviceCallDone_t;

/// WebserviceCallDone storing std::function
class WebserviceCallDoneFunction : public WebserviceCallDone {
public:
  typedef std::function<void (RestTransferStatus_t, WebserviceReply)> Function;
  WebserviceCallDoneFunction(Function &&f) : m_f(std::move(f)) {}
  void done(RestTransferStatus_t status, WebserviceReply reply) /* override */ {
    m_f(status, reply);
  }
private:
  Function m_f;
};

// --------------------------------------------------------------------
// interface calls

class WebserviceMsHub {
  __DECL_LOG_CHANNEL__
public:
  typedef enum {
    ApartmentChange = 1,
    TimedEventChange = 2,
    UDAChange = 3,
  } ChangeType;

  /**
   * Did the user authorize cloud service
   * @return
   */
  static bool isAuthorized();

  /**
   * doModelChanged - schedules asynchronous http request
   * @type: what part of the model changed
   * @callback: provides network and server status
   */
  static void doModelChanged(ChangeType type, WebserviceCallDone_t callback);

  static void doDssBackAgain(WebserviceCallDone_t callback);
  /**
   * Asynchronously notifies the cloud about removed application token
   * if you want to retry the request in case of network failure you can pass
   * request_done callback argument
   * @token: token to be deleted
   * @callback: provides network and server status
   */
  static void doNotifyTokenDeleted(const std::string &token,
                                   WebserviceCallDone_t callback);

  /**
   * Pull online weather information.
   * @callback: provides network and server status
   */
  static void doGetWeatherInformation(boost::shared_ptr<URLRequestCallback> callback);

  template <class iterator>
  static bool doUploadSensorData(iterator begin, iterator end,
                                 WebserviceCallDone_t callback);

  static void doVdcStoreVdcToken(const std::string &vdcToken, WebserviceCallDone_t callback);

  /**
   * Extract return code and message
   * @param json -- json encoded reply
   * @return decodod struct or throw ParseError if failed
   */
  static WebserviceReply parseReply(const char* buf);
};


namespace DsHub {
  std::vector<std::string> uploadEvents();
  void toJson(const boost::shared_ptr<Event> &event, JSONWriter& json);
}

class WebserviceDsHub {
  __DECL_LOG_CHANNEL__
public:
  /**
   * Checks whether the user authorize cloud service.
   */
  static bool isAuthorized();

  template <class iterator>
  static bool doUploadSensorData(iterator begin, iterator end,
                                 WebserviceCallDone_t callback);
};

}

#endif
