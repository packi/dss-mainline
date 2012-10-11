/*
    Copyright (c) 2012 digitalSTROM.org, Zurich, Switzerland

    Author: Sergey 'Jin' Bostandzhyan <jin@dev.digitalstrom.org>

    This file is part of digitalSTROM Server.

    digitalSTROM Server is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    digitalSTROM Server is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with digitalSTROM Server. If not, see <http://www.gnu.org/licenses/>.

*/

#include "config.h"

#ifdef HAVE_CURL

#include <curl/curl.h>

#include "logger.h"
#include "url.h"

namespace dss {

URL::URL() {}

long URL::request(std::string url, bool HTTP_POST) {
  CURLcode res;
  char error_buffer[CURL_ERROR_SIZE] = {'\0'};
  long http_code = -1;

  CURL *curl_handle = curl_easy_init();
  curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
  if (HTTP_POST) {
    curl_easy_setopt(curl_handle, CURLOPT_HTTPPOST, 1);
  } else {
    curl_easy_setopt(curl_handle, CURLOPT_HTTPGET, 1);
  }

  curl_easy_setopt(curl_handle, CURLOPT_ERRORBUFFER, error_buffer);

  res = curl_easy_perform(curl_handle);
  if (res != CURLE_OK) {
    Logger::getInstance()->log(std::string("URL::request: ") + error_buffer);
    curl_easy_cleanup(curl_handle);
    return http_code;
  }

  curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &http_code);
  curl_easy_cleanup(curl_handle);
  return http_code;
}

}

#endif//HAVE_CURL

