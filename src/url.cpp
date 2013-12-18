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

#include "foreach.h"
#include "logger.h"
#include "url.h"
#include "base.h"

#define CURL_TRANSFER_TIMEOUT_SECS 2*60 // 2 minutes

namespace dss {

HashMapStringString URL::emptyHeader;
HashMapStringString URL::emptyForm;

URL::URL() {}

URLResult::~URLResult()
{
  reset();
}

void URLResult::reset()
{
  if (m_size) {
    free(m_memory);
  }
  m_size = 0;
  m_memory = NULL;
}

const char *URLResult::content()
{
  if (!m_size) {
    return NULL;
  }
  return m_memory;
}

void *URLResult::grow_tail(size_t increase)
{
  m_memory = (char *)realloc(m_memory, m_size + increase + 1);
  if (!m_memory) {
    return NULL;
  }
  return &m_memory[m_size];
}

size_t URLResult::appendCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
  size_t realsize = size * nmemb;
  URLResult *mem = (URLResult *)userp;

  void *space = mem->grow_tail(realsize);
  if (space == NULL) {
    /* out of memory! */
    Logger::getInstance()->log(std::string("URL::appendCallback: (realloc failed)\n"));
    return -1;
  }

  memcpy(space, contents, realsize);
  mem->m_size += realsize;
  mem->m_memory[mem->m_size] = 0;

  return realsize;
}

size_t URL::writeCallbackMute(void* contents, size_t size, size_t nmemb, void* userp)
{
  /* throw it away */
  return size * nmemb;
}

long URL::request(const std::string& url, RequestType type, class URLResult* result)
{
  return request(url, type, HashMapStringString(), HashMapStringString(), result);
}

long URL::request(const std::string& url, RequestType type,
                  const HashMapStringString &headers,
                  const HashMapStringString &formpost,
                  URLResult* result)
{
  CURLcode res;
  char error_buffer[CURL_ERROR_SIZE] = {'\0'};
  struct curl_slist *cheaders = NULL;
  struct curl_httppost *formpost_start = NULL;
  struct curl_httppost *formpost_end = NULL;
  long http_code = -1;

  CURL *curl_handle = curl_easy_init();
  curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());

  if (!headers.empty()) {
    foreach(HashMapStringString::value_type elt, headers) {
      cheaders = curl_slist_append(cheaders, (elt.first + ": " + elt.second).c_str());
    }
    curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, cheaders);
  }

  switch (type) {
  case GET:
    curl_easy_setopt(curl_handle, CURLOPT_HTTPGET, 1);
    curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
    break;
  case POST:
    curl_easy_setopt(curl_handle, CURLOPT_POST, 1L);
    foreach(HashMapStringString::value_type elt, formpost) {
      curl_formadd(&formpost_start, &formpost_end,
                   CURLFORM_COPYNAME, elt.first.c_str(),
                   CURLFORM_COPYCONTENTS, elt.second.c_str(),
                   CURLFORM_END);
    }
    /* must be set even if it's null */
    curl_easy_setopt(curl_handle, CURLOPT_HTTPPOST, formpost_start);
    break;
  }

  if (result != NULL) {
    /* send all data to this function  */
    result->reset();
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, URLResult::appendCallback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)result);
  } else {
    /* suppress output to stdout */
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, URL::writeCallbackMute);
  }
  curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, CURL_TRANSFER_TIMEOUT_SECS);
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

  if (formpost_start) {
    curl_formfree(formpost_start);
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

