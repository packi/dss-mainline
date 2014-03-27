#ifndef __WEBSERVICE_API__
#define __WEBSERVICE_API__

#include <stdexcept>

namespace dss {

struct ModelChangeResponse {
    int code;
    std::string desc;
};

class ParseError : public std::runtime_error {
public:
    ParseError(const std::string& _message);
};

ModelChangeResponse parseModelChange(const char* buf);

}

#endif
