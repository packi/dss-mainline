#include "bonjour.h"
#include "propertysystem.h"
#include "dss.h"
#include "base.h"

#include <arpa/inet.h>

#include <iostream>
#include <stdexcept>

using namespace std;

namespace dss {

  BonjourHandler::BonjourHandler()
    : Thread("BonjourHandler")
  {
  } // ctor


  BonjourHandler::~BonjourHandler()
  { } // dtor

  void RegisterCallback(
    DNSServiceRef sdRef,
    DNSServiceFlags flags,
    DNSServiceErrorType errorCode,
    const char *name,
    const char *regtype,
    const char *domain,
    void *context )
  {
    if(errorCode != kDNSServiceErr_NoError) {
      cerr << "error received in browse callback" << endl;
    }
  } // RegisterCallback

  const char* TheMDNSRegisterType = "_dssweb._tcp";

  void BonjourHandler::Execute() {

    int serverPort = StrToInt(DSS::GetInstance()->GetPropertySystem().GetStringValue("/config/subsystems/WebServer/ports"));

    DNSServiceErrorType err;

    memset(&m_RegisterReference, '\0', sizeof(m_RegisterReference));

    err =
    DNSServiceRegister(
      &m_RegisterReference,
      kDNSServiceFlagsDefault,
      0,
      NULL,
      TheMDNSRegisterType,
      NULL,
      NULL,
      htons(serverPort),
      0,
      NULL,
      &RegisterCallback,
      NULL
    );

    if(err != kDNSServiceErr_NoError) {
      throw runtime_error("error registering service");
    }
  } // Execute

}
