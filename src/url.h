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

#ifndef __URL_H__
#define __URL_H__

#include <curl/curl.h>

#include "base.h"
#include "logger.h"

namespace dss {

  typedef enum {
    GET,
    POST,
  } RequestType;

  typedef boost::shared_ptr<HashMapStringString> headers_t;
  typedef boost::shared_ptr<HashMapStringString> formpost_t;

  class URLResult {
  public:
      friend class URL;
      URLResult() : m_memory(NULL), m_size(0) {}
      virtual ~URLResult();

      void reset();
      void *grow_tail(size_t increase);
      const char* content();

  private:
      static size_t appendCallback(void* contents, size_t size, size_t nmemb, void* userp);
      char* m_memory;
      size_t m_size;
  };

  // TODO rename to HttpClient
  class URL {
    __DECL_LOG_CHANNEL__
    public:

      URL(bool _reuse_handle = false);
      ~URL();

      long request(const std::string& url, RequestType type = GET,
                   URLResult* result = NULL);

      long request(const std::string& url,
                   boost::shared_ptr<HashMapStringString> headers,
                   std::string postdata, URLResult* result);

      long request(const std::string& url, RequestType type,
                   boost::shared_ptr<HashMapStringString> headers,
                   boost::shared_ptr<HashMapStringString> formpost,
                   URLResult* result);

      long downloadFile(std::string url, std::string filename);

      /* TODO evtl. URI wrapper easy appending/parsing of query options */

    private:
      long internalRequest(const std::string& url, RequestType type,
                   std::string postdata,
                   boost::shared_ptr<HashMapStringString> headers,
                   boost::shared_ptr<HashMapStringString> formpost,
                   URLResult* result);

      static size_t writeCallbackMute(void* contents, size_t size, size_t nmemb, void* userp);
      bool m_reuse_handle;
      CURL *m_curl_handle;
  };
};

#endif//__URL_H__
