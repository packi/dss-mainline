/*
    Copyright (c) 2009,2012 digitalSTROM.org, Zurich, Switzerland

    Author: Patrick Staehlin, futureLAB AG <pstaehlin@futurelab.ch>
            Christian Hitz, aizo AG <christian.hitz@aizo.com>

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


#include "restfulapiwriter.h"
#include "src/foreach.h"

#include <fstream>

namespace dss {

  void RestfulAPIWriter::writeToXML(const RestfulAPI& api, const std::string& _location) {
    int indent = 0;
    std::string document = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
    document += "<?xml-stylesheet type=\"text/xml\" href=\"json_api.xslt\"?>\n";

    document += doIndent(indent) + "<api>\n";
    document += doIndent(++indent) + "<classes>\n";

    foreach(const RestfulClass& cls, api.getClasses()) {
      document += doIndent(++indent) + "<class>\n";
      document += doIndent(++indent) + "<name>" + XMLStringEscape(cls.getName()) + "</name>\n";

      if (cls.getInstanceParameter().empty()) {
        document += doIndent(indent) + "<instanceParameter/>\n";
      } else {
        document += doIndent(indent) + "<instanceParameter>\n";
        foreach(const RestfulParameter& instanceParam, cls.getInstanceParameter()) {
          writeToXML(instanceParam, document, indent + 1);
        }
        document += doIndent(indent) + "</instanceParameter>\n";
      }

      if((!cls.getDocumentationShort().empty()) || (!cls.getDocumentationLong().empty())) {
        document += doIndent(indent) + "<documentation>\n";
        if(!cls.getDocumentationShort().empty()) {
          document += doIndent(indent + 1) + "<short>" + XMLStringEscape(cls.getDocumentationShort()) + "</short>\n";
        }
        if(!cls.getDocumentationLong().empty()) {
          document += doIndent(indent + 1) + "<long>" + XMLStringEscape(cls.getDocumentationLong()) + "</long>\n";
        }
        document += doIndent(indent) + "</documentation>\n";
      }

      if (cls.getMethods().empty()) {
        document += doIndent(indent) + "<methods/>\n";
      } else {
        document += doIndent(indent) + "<methods>\n";
        foreach(const RestfulMethod& method, cls.getMethods()) {
          document += doIndent(++indent) + "<method>\n";
          document += doIndent(++indent) + "<name>" + XMLStringEscape(method.getName()) + "</name>\n";

          if((!method.getDocumentationShort().empty()) || (!method.getDocumentationLong().empty())) {
            document += doIndent(indent) + "<documentation>\n";
            if(!method.getDocumentationShort().empty()) {
              document += doIndent(indent + 1) + "<short>" + XMLStringEscape(method.getDocumentationShort()) + "</short>\n";
            }
            if(!method.getDocumentationLong().empty()) {
              document += doIndent(indent + 1) + "<long>" + XMLStringEscape(method.getDocumentationLong()) + "</long>\n";
            }
            document += doIndent(indent) + "</documentation>\n";
          }

          if (method.getParameter().empty()) {
            document += doIndent(indent) + "<parameter/>\n";
          } else {
            document += doIndent(indent) + "<parameter>\n";
            foreach(const RestfulParameter& parameter, method.getParameter()) {
              writeToXML(parameter, document, indent + 1);
            }
            document += doIndent(indent) + "</parameter>\n";
          }

          document += doIndent(--indent) + "</method>\n";
          --indent;
        }
        document += doIndent(indent) + "</methods>\n";
      }

      document += doIndent(--indent) + "</class>\n";
      --indent;
    }

    document += doIndent(indent) + "</classes>\n";
    document += doIndent(--indent) + "</api>\n";

    std::ofstream ofs(_location.c_str());
    if(ofs) {
      ofs << document;
      ofs.close();
    }
  } // writeToXML

  void RestfulAPIWriter::writeToXML(const RestfulParameter& _parameter, std::string& _document, const int _indent) {
    _document += doIndent(_indent) + "<parameter>\n";
    _document += doIndent(_indent + 1) + "<name>" + XMLStringEscape(_parameter.getName()) + "</name>\n";
    _document += doIndent(_indent + 1) + "<type>" + XMLStringEscape(_parameter.getTypeName()) + "</type>\n";
    _document += doIndent(_indent + 1) + "<required>" + (_parameter.isRequired() ? "true" : "false") + "</required>\n";
    _document += doIndent(_indent) + "</parameter>\n";
  } // writeToXML


} // namespace dss

