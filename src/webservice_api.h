#ifndef __WEBSERVICE_API__
#define __WEBSERVICE_API__

#include <stdexcept>

namespace dss {

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

}

#endif
