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

#ifdef HAVE_CURL

#include "base.h"

namespace dss {

  typedef enum {
    GET,
    POST,
  } RequestType;

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

  class URL {
    public:

      static HashMapStringString emptyHeader;
      static HashMapStringString emptyForm;

      URL();

      long request(const std::string& url, RequestType type = GET,
                   struct URLResult* result = NULL);

      long request(const std::string& url, RequestType type,
                   const HashMapStringString &headers,
                   const HashMapStringString &formpost,
                   struct URLResult* result);

      long downloadFile(std::string url, std::string filename);

      /* TODO evtl. URI wrapper easy appending/parsing of query options */

    private:
      static size_t writeCallbackMute(void* contents, size_t size, size_t nmemb, void* userp);
  };
};

#endif//HAVE_CURL

#endif//__URL_H__
