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

#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>

#include "foreach.h"
#include "logger.h"
#include "url.h"
#include "base.h"

#define CURL_DEEP_DEBUG 0

/* TODO move to propertysystem_common_paths */
#define CURL_TRANSFER_TIMEOUT_SECS 2*60 // 2 minutes

namespace dss {

/**
 * TODO drop _reuse_handle, and reuse by default
 * follow RAII principle
 */
URL::URL(bool _reuse_handle) : m_reuse_handle(_reuse_handle),
                               m_curl_handle(NULL) {}
URL::~URL()
{
  if (m_curl_handle) {
      curl_easy_cleanup(m_curl_handle);
      m_curl_handle = NULL;
  }
}

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

__DEFINE_LOG_CHANNEL__(URL, lsInfo)

size_t URL::writeCallbackMute(void* contents, size_t size, size_t nmemb, void* userp)
{
  /* throw it away */
  return size * nmemb;
}

long URL::request(const std::string& url, RequestType type, class URLResult* result)
{
  return internalRequest(url, type, std::string(), headers_t(), formpost_t(),
                         result);
}

long URL::request(const std::string& url,
                  boost::shared_ptr<HashMapStringString> headers,
                  std::string postdata, class URLResult* result)
{
  return internalRequest(url, POST, postdata, headers, formpost_t(), result);
}

long URL::request(const std::string& url, RequestType type,
                  boost::shared_ptr<HashMapStringString> headers,
                  boost::shared_ptr<HashMapStringString> formpost,
                  URLResult* result)
{
  return internalRequest(url, type, std::string(), headers, formpost, result);
}

long URL::request(const HttpRequest &req, URLResult *result) {
  return internalRequest(req.url, req.type, req.postdata, req.headers,
                         req.formpost, result);
}

#if CURL_DEEP_DEBUG
struct data {
  char trace_ascii; /* 1 or 0 */
};

static
void dump(const char *text,
          FILE *stream, unsigned char *ptr, size_t size,
          char nohex)
{
  size_t i;
  size_t c;

  unsigned int width = 0x10;

  if (nohex) {
    /* without the hex output, we can fit more on screen */
    width = 0x40;
  }

  fprintf(stream, "%s, %10.10ld bytes (0x%8.8lx)\n",
          text, (long)size, (long)size);

  for (i = 0; i < size; i += width) {

    fprintf(stream, "%4.4lx: ", (long)i);

    if (!nohex) {
      /* hex not disabled, show it */
      for (c = 0; c < width; c++)
        if (i + c < size) {
          fprintf(stream, "%02x ", ptr[i + c]);
        } else {
          fputs("   ", stream);
        }
    }

    for (c = 0; (c < width) && (i + c < size); c++) {
      /* check for 0D0A; if found, skip past and start a new line of output */
      if (nohex && (i + c + 1 < size) && ptr[i + c] == 0x0D && ptr[i + c + 1] == 0x0A) {
        i += (c + 2 - width);
        break;
      }
      fprintf(stream, "%c",
              (ptr[i + c] >= 0x20) && (ptr[i + c] < 0x80) ? ptr[i + c] : '.');
      /* check again for 0D0A, to avoid an extra \n if it's at width */
      if (nohex && (i + c + 2 < size) && ptr[i + c + 1] == 0x0D && ptr[i + c + 2] == 0x0A) {
        i += (c + 3 - width);
        break;
      }
    }
    fputc('\n', stream); /* newline */
  }
  fflush(stream);
}

static int my_trace(CURL *handle, curl_infotype type,
                    char *data, size_t size,
                    void *userp)
{
  struct data *config = (struct data *)userp;
  const char *text;
  (void)handle; /* prevent compiler warning */

  switch (type) {
  case CURLINFO_TEXT:
    fprintf(stderr, "== Info: %s", data);
  default: /* in case a new one is introduced to shock us */
    return 0;

  case CURLINFO_HEADER_OUT:
    text = "=> Send header";
    break;
  case CURLINFO_DATA_OUT:
    text = "=> Send data";
    break;
  case CURLINFO_SSL_DATA_OUT:
    text = "=> Send SSL data";
    break;
  case CURLINFO_HEADER_IN:
    text = "<= Recv header";
    break;
  case CURLINFO_DATA_IN:
    text = "<= Recv data";
    break;
  case CURLINFO_SSL_DATA_IN:
    text = "<= Recv SSL data";
    break;
  }

  dump(text, stderr, (unsigned char *)data, size, config->trace_ascii);
  return 0;
}

struct data config;
#endif

long URL::internalRequest(const std::string& url, RequestType type,
                  std::string postdata,
                  boost::shared_ptr<HashMapStringString> headers,
                  boost::shared_ptr<HashMapStringString> formpost,
                  URLResult* result)
{
  CURLcode res;
  char error_buffer[CURL_ERROR_SIZE] = {'\0'};
  struct curl_slist *cheaders = NULL;
  struct curl_httppost *formpost_start = NULL;
  struct curl_httppost *formpost_end = NULL;
  long http_code = -1;

  if (!m_curl_handle) {
    log("create new handle", lsDebug);
    m_curl_handle = curl_easy_init();
  } else {
    log("reuse handle", lsDebug);
    curl_easy_reset(m_curl_handle);
  }

#if CURL_DEEP_DEBUG
  config.trace_ascii = 1; /* enable ascii tracing */
  curl_easy_setopt(m_curl_handle, CURLOPT_DEBUGFUNCTION, my_trace);
  curl_easy_setopt(m_curl_handle, CURLOPT_DEBUGDATA, &config);
  curl_easy_setopt(m_curl_handle, CURLOPT_VERBOSE, 1);
#endif

  curl_easy_setopt(m_curl_handle, CURLOPT_URL, url.c_str());

  HashMapStringString::iterator it;

  if (headers.get() && !headers->empty()) {
    for (it = headers->begin(); it != headers->end(); it++) {
      cheaders = curl_slist_append(cheaders, (it->first + ": " + it->second).c_str());
    }
    curl_easy_setopt(m_curl_handle, CURLOPT_HTTPHEADER, cheaders);
  }

  switch (type) {
  case GET:
    curl_easy_setopt(m_curl_handle, CURLOPT_HTTPGET, 1);
    curl_easy_setopt(m_curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
    break;
  case POST:
    curl_easy_setopt(m_curl_handle, CURLOPT_POST, 1L);
    if (!postdata.empty()) {
      curl_easy_setopt(m_curl_handle, CURLOPT_COPYPOSTFIELDS, postdata.c_str());
    } else if (formpost.get() && !formpost->empty()) {
      for (it = formpost->begin(); it != formpost->end(); it++) {
        curl_formadd(&formpost_start, &formpost_end,
                     CURLFORM_COPYNAME, it->first.c_str(),
                     CURLFORM_COPYCONTENTS, it->second.c_str(),
                     CURLFORM_END);
      }
      curl_easy_setopt(m_curl_handle, CURLOPT_HTTPPOST, formpost_start);
    } else {
      // empty post is valid request, but not without this call
      curl_easy_setopt(m_curl_handle, CURLOPT_HTTPPOST, NULL);
    }
    break;
  }

  if (result != NULL) {
    /* send all data to this function  */
    result->reset();
    curl_easy_setopt(m_curl_handle, CURLOPT_WRITEFUNCTION, URLResult::appendCallback);
    curl_easy_setopt(m_curl_handle, CURLOPT_WRITEDATA, (void *)result);
  } else {
    /* suppress output to stdout */
    curl_easy_setopt(m_curl_handle, CURLOPT_WRITEFUNCTION, URL::writeCallbackMute);
  }
  curl_easy_setopt(m_curl_handle, CURLOPT_TIMEOUT, CURL_TRANSFER_TIMEOUT_SECS);
  curl_easy_setopt(m_curl_handle, CURLOPT_ERRORBUFFER, error_buffer);

  log("perform: " + std::string((type == POST) ? "POST " : " ") + url, lsDebug);
  res = curl_easy_perform(m_curl_handle);
  if (res != CURLE_OK) {
    log(std::string("request: ") + error_buffer, lsError);
    if (!m_reuse_handle) {
      curl_easy_cleanup(m_curl_handle);
      m_curl_handle = NULL;
    }
    return http_code;
  }

  if (cheaders) {
    curl_slist_free_all(cheaders);
  }

  if (formpost_start) {
    curl_formfree(formpost_start);
  }

  curl_easy_getinfo(m_curl_handle, CURLINFO_RESPONSE_CODE, &http_code);
  log("return code: " + intToString(http_code), lsDebug);
    
  if (!m_reuse_handle) {
    curl_easy_cleanup(m_curl_handle);
    m_curl_handle = NULL;
  }
  return http_code;
}

long URL::downloadFile(std::string url, std::string filename) {
  CURLcode res;
  char error_buffer[CURL_ERROR_SIZE] = {'\0'};
  long http_code = -1;
  FILE *data;

  if (!m_curl_handle) {
    m_curl_handle = curl_easy_init();
  }
  curl_easy_setopt(m_curl_handle, CURLOPT_URL, url.c_str());
  curl_easy_setopt(m_curl_handle, CURLOPT_HTTPGET, 1);

  curl_easy_setopt(m_curl_handle, CURLOPT_ERRORBUFFER, error_buffer);

  data = fopen(filename.c_str(), "w");
  if (data == NULL) {
    if (!m_reuse_handle) {
      curl_easy_cleanup(m_curl_handle);
      m_curl_handle = NULL;
    }
    return http_code;
  }

  curl_easy_setopt(m_curl_handle, CURLOPT_WRITEDATA, data);

  log("download : " + url, lsDebug);
  res = curl_easy_perform(m_curl_handle);
  if (res != CURLE_OK) {
    log(std::string("request: ") + error_buffer, lsError);
    fclose(data);
    if (!m_reuse_handle) {
      curl_easy_cleanup(m_curl_handle);
      m_curl_handle = NULL;
    }
    return http_code;
  }

  curl_easy_getinfo(m_curl_handle, CURLINFO_RESPONSE_CODE, &http_code);
  log("return code: " + intToString(http_code), lsDebug);

  fclose(data);
  if (!m_reuse_handle) {
    curl_easy_cleanup(m_curl_handle);
    m_curl_handle = NULL;
  }
  return http_code;
}

}
