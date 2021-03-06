/*
    Copyright (c) 2012, 2014 digitalSTROM.org, Zurich, Switzerland

    Authors: Sergey 'Jin' Bostandzhyan <jin@dev.digitalstrom.org>
             Andreas Fenkart <andreas.fenkart@dev.digitalstrom.org>

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

#include <unordered_map>

#include <curl/curl.h>

#include "logger.h"

namespace dss {

  typedef enum {
    GET,
    POST,
  } RequestType;

  /**
   * HttpRequest - Combine all data describing an HTTP request
   * @url: http://www.something.org/path/to/something?query1=foo&query2=bar
   * @type: POST|GET
   * @header: will be unrolled as 'tag: value' form
   * @formpost: will be unrolled as 'tag: value' form
   * @postdata: preformatted postdata, has precedence over formpost
   */
  struct HttpRequest {
    std::string url;
    RequestType type;
    std::unordered_map<std::string, std::string> headers;
    std::string postdata;
  };

  struct HttpStatistics {
    double totalTime;
    double sumUp, sumDown;
    double speedUp, speedDown;
  };

  class HttpClient {
    __DECL_LOG_CHANNEL__
    public:

      HttpClient(bool _reuse_handle = false);
      ~HttpClient();

      long get(const std::string& url, std::string *result,
               bool insecure = false);

      long get(const std::string& url,
               const std::unordered_map<std::string, std::string>& headers,
               std::string *result, bool insecure = false);

      long post(const std::string& _url, const std::string &_postdata,
                std::string *_result);

      /* TODO make postdata const-by-reference */
      long post(const std::string& _url,
                const std::unordered_map<std::string, std::string>& headers,
                const std::string &_postdata, std::string *_result);

      long request(const HttpRequest &req, std::string *result);

      long downloadFile(const std::string &_url, const std::string &_filename);

      /* TODO evtl. URI wrapper easy appending/parsing of query options */

      HttpStatistics getStats();

    private:
      long internalRequest(const std::string& _url, RequestType _type,
                           const std::unordered_map<std::string, std::string>& headers,
                           const std::string &_postdata,
                           std::string *_result, bool _insecure);

      static size_t writeCallbackMute(void* contents, size_t size, size_t nmemb, void* userp);
      bool m_reuse_handle;
      CURL *m_curl_handle;
  };
};

#endif//__URL_H__
