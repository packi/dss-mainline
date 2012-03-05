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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sstream>
#include "expatparser.h"
#include "logger.h"

#define XML_PARSER_BUFFER_SIZE 256

namespace dss {
    
  ExpatParser::ExpatParser() : m_forceStop(false) {}

  bool ExpatParser::parseFile(const std::string& _fileName)
  {
    char buffer[XML_PARSER_BUFFER_SIZE];
    m_forceStop = false;

    XML_Parser parser = XML_ParserCreate(NULL);
    if (parser == NULL) {
      Logger::getInstance()->log("ExpatParser::parseFile: Couldn't "
                                 "allocate memory for parser", lsError);
      return false;
    }
      
    // setup callbacks
    XML_SetUserData(parser, this);
    XML_SetElementHandler(parser, ExpatParser::expatElStart,
                          ExpatParser::expatElEnd);
    XML_SetCharacterDataHandler(parser, ExpatParser::expatCharData);

    FILE *f = fopen(_fileName.c_str(), "r");
    if (f == NULL) {
      Logger::getInstance()->log(std::string("ExpatParser::parseFile: "
                                 "Could not open file ") + _fileName, lsError);
      XML_ParserFree(parser);
      return false;
    }

    int end = 0;
    do
    {
      size_t len = fread(buffer, 1, XML_PARSER_BUFFER_SIZE, f);
      if (ferror(f)) {
        Logger::getInstance()->log(std::string("ExpatParser::parseFile: "
                                   "Error when reading file ") + _fileName,
                                   lsError);
        fclose(f);
        XML_ParserFree(parser);
        return false;
      }
      end = feof(f);

      if (XML_Parse(parser, buffer, (int)len, end) != XML_STATUS_OK) {
        std::stringstream err;
        err << "Parse error at line: " << XML_GetCurrentLineNumber(parser);
        err << ", column " << XML_GetCurrentColumnNumber(parser) << " : ";
        err << XML_ErrorString(XML_GetErrorCode(parser));
        Logger::getInstance()->log(err.str(), lsError);
        fclose(f);
        XML_ParserFree(parser);
        return false;
      }

      if (m_forceStop) {
        break;
      }
    } while (!end);
    
    fclose(f);
    XML_ParserFree(parser);
    return (!m_forceStop);
  }

  void XMLCALL ExpatParser::expatElStart(void *_userdata, const char *_name,
                                         const char **_attrs) {
    ExpatParser *ep = (ExpatParser *)_userdata;
    try {
      ep->elementStart(_name, _attrs);
    } catch (...) {
      ep->m_forceStop = true;
      Logger::getInstance()->log("ExpatParser::expatElStart: unhandled "
                                 "exception");
    }
  }

  void XMLCALL ExpatParser::expatElEnd(void *_userdata, const char *_name) {
    ExpatParser *ep = (ExpatParser *)_userdata;
    try {
      ep->elementEnd(_name);
    } catch (...) {
      ep->m_forceStop = true;
      Logger::getInstance()->log("ExpatParser::expatElEnd: unhandled "
                                 "exception");
    }
  }

  void XMLCALL ExpatParser::expatCharData(void *_userdata, const XML_Char *_s,
                                          int _len) {
    ExpatParser *ep = (ExpatParser *)_userdata;
    try {
      ep->characterData(_s, _len);
    } catch (...) {
      ep->m_forceStop = true;
      Logger::getInstance()->log("ExpatParser::expatCharData: unhandled "
                                 "exception");
    }
  }
};

