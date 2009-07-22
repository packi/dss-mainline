#include <stdlib.h>

#include "../../../core/web/plugin/webserver_plugin.h"

int plugin_getversion() {
  return WEBSERVER_PLUGIN_API_VERSION;
}

bool plugin_handlerequest(const std::string& _uri, dss::HashMapConstStringString& _parameter, std::string& result) {
  result = "echo... from: " + _uri;
  return true;
}

