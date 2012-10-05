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

namespace dss {
  class URL {
    public:
      struct URLResult {
        char* memory;
        size_t size;
        URLResult() : memory(NULL), size(0) {}
      };
      URL();
      long request(std::string url, bool HTTP_POST = false, struct URLResult* result = NULL);
      long downloadFile(std::string url, std::string filename);
    private:
      static size_t writeMemoryCallback(void* contents, size_t size, size_t nmemb, void* userp);
  };
};

#endif//HAVE_CURL

#endif//__URL_H__
