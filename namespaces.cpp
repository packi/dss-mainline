// Do not remove the following define as it takes out the previous definition of namespaces
#define WITH_NONAMESPACES
#include "webservices/soapH.h"
#undef WITH_NONAMESPACES


extern "C" {
  struct Namespace namespaces[] =
  {
      {"SOAP-ENV", "http://schemas.xmlsoap.org/soap/envelope/", "http://www.w3.org/*/soap-envelope", NULL},
      {"SOAP-ENC", "http://schemas.xmlsoap.org/soap/encoding/", "http://www.w3.org/*/soap-encoding", NULL},
      {"xsi", "http://www.w3.org/2001/XMLSchema-instance", "http://www.w3.org/*/XMLSchema-instance", NULL},
      {"xsd", "http://www.w3.org/2001/XMLSchema", "http://www.w3.org/*/XMLSchema", NULL},
      {"dss", "urn:dss:1.0", NULL, NULL},
      {NULL, NULL, NULL, NULL}
  };

}
