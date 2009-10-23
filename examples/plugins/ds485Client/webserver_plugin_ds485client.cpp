#include <cstdlib>
#include <iostream>

#include "core/base.h"
#include "core/web/plugin/webserver_plugin.h" 
#include "core/datetools.h" 
#include "core/dss.h" 
#include "core/model.h" 
#include "core/ds485client.h" 
#include "unix/ds485.h" 
#include "core/ds485const.h" 

using namespace dss;

int plugin_getversion() {
  return WEBSERVER_PLUGIN_API_VERSION;
}

bool plugin_handlerequest(const std::string& _uri, dss::HashMapConstStringString& _parameter, std::string& result) {
  std::cout << "in plugin_handlerequest" << std::endl;
  if(endsWith(_uri, "/send")) {
    DS485Client oClient;

    int destination = strToIntDef(_parameter["destination"],0) & 0x3F;
    bool broadcast = _parameter["broadcast"] == "true";
    int counter = strToIntDef(_parameter["counter"], 0x00) & 0x03;
    int command = strToIntDef(_parameter["command"], 0x09 /* request */) & 0x00FF;
    int length = strToIntDef(_parameter["length"], 0x00) & 0x0F;

    std::cout
         << "sending frame: "
         << "\ndest:    " << destination
         << "\nbcst:    " << broadcast
         << "\ncntr:    " << counter
         << "\ncmd :    " << command
         << "\nlen :    " << length << std::endl;

    DS485CommandFrame frame;
    frame.getHeader().setBroadcast(broadcast);
    frame.getHeader().setDestination(destination);
    frame.getHeader().setCounter(counter);
    frame.setCommand(command);
    for(int iByte = 0; iByte < length; iByte++) {
      uint8_t byte = strToIntDef(_parameter[std::string("payload_") + intToString(iByte+1)], 0xFF);
      std::cout << "b" << std::dec << iByte << ": " << std::hex << (int)byte << "\n";
      frame.getPayload().add<uint8_t>(byte);
    }
    std::cout << std::dec << "done" << std::endl;

    oClient.sendFrameDiscardResult(frame); 
    result = "done.";
  } else {
    result = "echo... from: " + _uri;
  }
  return true;
}

