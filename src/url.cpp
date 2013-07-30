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
#include <stdlib.h>
#include <string.h>

#include "logger.h"
#include "url.h"
#include "base.h"

namespace dss {

URL::URL() {}

size_t URL::writeMemoryCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
  size_t realsize = size * nmemb;
  struct URLResult *mem = (struct URLResult *)userp;

  mem->memory = (char *)realloc(mem->memory, mem->size + realsize + 1);
  if (mem->memory == NULL) {
    /* out of memory! */
    Logger::getInstance()->log(std::string("URL::writeMemoryCallback: not enough memory (realloc returned NULL)\n"));
    return -1;
  }

  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}

long URL::request(const std::string& url, RequestType type, struct URLResult* result)
{
  return request(url, type, NULL, result);
}

long URL::request(const std::string& url, RequestType type,
                  const HashMapStringString* headers,
                  struct URLResult* result)
{
  CURLcode res;
  char error_buffer[CURL_ERROR_SIZE] = {'\0'};
  struct curl_slist *cheaders = NULL;
  long http_code = -1;

  CURL *curl_handle = curl_easy_init();
  curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());

  switch (type) {
  case GET:
    curl_easy_setopt(curl_handle, CURLOPT_HTTPGET, 1);
    break;
  case POST:
    curl_easy_setopt(curl_handle, CURLOPT_HTTPPOST, 1);
    break;
  }

  if (headers && !headers->empty()) {
    for (HashMapStringString::const_iterator it = headers->begin();
         it != headers->end(); it++) {
      cheaders = curl_slist_append(cheaders, (it->first + ": " + it->second).c_str());
    }
    curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, cheaders);
  }

  if (result != NULL) {
    /* send all data to this function  */
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, URL::writeMemoryCallback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)result);
  }

  curl_easy_setopt(curl_handle, CURLOPT_ERRORBUFFER, error_buffer);
  res = curl_easy_perform(curl_handle);
  if (res != CURLE_OK) {
    Logger::getInstance()->log(std::string("URL::request: ") + error_buffer);
    curl_easy_cleanup(curl_handle);
    return http_code;
  }

  if (cheaders) {
    curl_slist_free_all(cheaders);
  }

  curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &http_code);
  curl_easy_cleanup(curl_handle);
  return http_code;
}

long URL::downloadFile(std::string url, std::string filename) {
  CURLcode res;
  char error_buffer[CURL_ERROR_SIZE] = {'\0'};
  long http_code = -1;
  FILE *data;

  CURL *curl_handle = curl_easy_init();
  curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl_handle, CURLOPT_HTTPGET, 1);

  curl_easy_setopt(curl_handle, CURLOPT_ERRORBUFFER, error_buffer);

  data = fopen(filename.c_str(), "w");
  if (data == NULL) {
    curl_easy_cleanup(curl_handle);
    return http_code;
  }

  curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, data);

  res = curl_easy_perform(curl_handle);
  if (res != CURLE_OK) {
    Logger::getInstance()->log(std::string("URL::request: ") + error_buffer);
    fclose(data);
    curl_easy_cleanup(curl_handle);
    return http_code;
  }

  curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &http_code);
  fclose(data);
  curl_easy_cleanup(curl_handle);
  return http_code;
}

}

#endif//HAVE_CURL

