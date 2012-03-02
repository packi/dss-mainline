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

#ifndef __DSS_EXPAT_PARSER_H__
#define __DSS_EXPAT_PARSER_H__

#include <expat.h>
#include <string>

namespace dss {
  class ExpatParser
  {
  public:
    ExpatParser();
  protected:
    // set this variable to true in your callback if you encountered an
    // unrecoverable error, this will stop the file read out loop
    bool m_forceStop;

    bool parseFile(const std::string& _fileName);

    // implement your logic in these callbacks
    virtual void elementStart(const char *_name, const char **_attrs) = 0;
    virtual void elementEnd(const char *_name) = 0;
    virtual void characterData(const XML_Char *_s, int _len) = 0;
  private:
    static void XMLCALL expatElStart(void *_userdata, const char *_name,
                                     const char **_attrs);
    static void XMLCALL expatElEnd(void *_userdata, const char *_name);
    static void XMLCALL expatCharData(void *_userdata, const XML_Char *_s,
                                      int _len);
  };
};

#endif
