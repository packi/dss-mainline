/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland

    Author: Patrick Staehlin, futureLAB AG <pstaehlin@futurelab.ch>

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

#include "seriespersistence.h"

#include "core/logger.h"
#include "core/foreach.h"

#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/Attr.h>
#include <Poco/DOM/ProcessingInstruction.h>
#include <Poco/DOM/Text.h>
#include <Poco/DOM/AutoPtr.h>
#include <Poco/DOM/DOMWriter.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/XML/XMLWriter.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/DOM/Node.h>
#include <Poco/SAX/InputSource.h>
#include <Poco/SAX/SAXException.h>

#include <iostream>
#include <fstream>
#include <sstream>


using Poco::XML::Document;
using Poco::XML::Element;
using Poco::XML::Attr;
using Poco::XML::ProcessingInstruction;
using Poco::XML::Text;
using Poco::XML::AutoPtr;
using Poco::XML::DOMWriter;
using Poco::XML::XMLWriter;

using Poco::XML::DOMParser;
using Poco::XML::InputSource;
using Poco::XML::NodeList;
using Poco::XML::Node;
using Poco::Exception;


namespace dss {

  const int SeriesXMLFileVersion = 1;

  //================================================== SeriesReader

  template<class T>
  Series<T>* SeriesReader<T>::readFromXML(const std::string& _fileName) {
    Timestamp parsing;

    std::ifstream inFile(_fileName.c_str());
    if(!inFile.is_open()) {
      Logger::getInstance()->log("SeriesReader::readFromXML: could not open file: '" + _fileName + "'");
      return NULL;
    }

    InputSource input(inFile);
    Series<T>* result = NULL;
    DOMParser parser;
    AutoPtr<Document> pDoc;
    try {
      pDoc = parser.parse(&input);
    } catch(Poco::XML::SAXParseException& e) {
      Logger::getInstance()->log("SeriesReader::readFromXML: error parsing '"
                                 + _fileName + "': " + e.what() , lsError);
      return NULL;
    }
    Element* rootNode = pDoc->documentElement();
    if(rootNode->localName() != "metering") {
      Logger::getInstance()->log("SeriesReader::readFromXML: root node must be named metering, got: '" + rootNode->localName() + "'");
      return NULL;
    }
    if(!rootNode->hasAttribute("version")) {
      Logger::getInstance()->log("SeriesReader::readFromXML: missing version attribute", lsError);
      return NULL;
    }

    if(strToIntDef(rootNode->getAttribute("version"),-1) != SeriesXMLFileVersion) {
      Logger::getInstance()->log(std::string("SeriesReader::readFromXML: Version mismatch, expected ") + intToString(SeriesXMLFileVersion) + " got " + rootNode->getAttribute("version"),
                                 lsError);
      return NULL;
    }

    Element* valuesNode = rootNode->getChildElement("values");
    int numberOfValues = strToInt(valuesNode->getAttribute("numberOfValues"));
    int resolution = strToInt(valuesNode->getAttribute("resolution"));
    std::string comment, fromdsid, unit, valtype;
    kValueType stype;

    Element* configElem = rootNode->getChildElement("config");
    if(configElem != NULL) {
      Element* elem = configElem->getChildElement("comment");
      if(elem != NULL && elem->hasChildNodes()) {
        comment = elem->firstChild()->getNodeValue();
      }
      elem = configElem->getChildElement("from_dsid");
      if(elem != NULL && elem->hasChildNodes()) {
        fromdsid = elem->firstChild()->getNodeValue();
      }
      elem = configElem->getChildElement("unit");
      if(elem != NULL && elem->hasChildNodes()) {
        unit = elem->firstChild()->getNodeValue();
      }
      elem = configElem->getChildElement("type");
      if(elem != NULL && elem->hasChildNodes()) {
        valtype = elem->firstChild()->getNodeValue();
        if (valtype.compare("average") == 0) {
          stype = kAverage;
        } else if (valtype.compare("maximum") == 0) {
          stype = kMaximum;
        } else if (valtype.compare("absolute") == 0) {
          stype = kAbsolute;
        }
      }
      else {
        // Backward compatibility
        if (comment.find("Consumption", 0) != std::string::npos) {
          stype = kAverage;
        } else if (comment.find("Energy", 0) != std::string::npos) {
          stype = kMaximum;
        }
      }
    }

    result = new Series<T>(resolution, numberOfValues, stype);
    result->setComment(comment);
    result->setFromDSID(dss_dsid_t::fromString(fromdsid));
    result->setUnit(unit);

    Node* node = valuesNode->firstChild();
    while(node != NULL) {
      if(node->nodeName() == "value") {
        try {
          T value(0);
          value.readFromXMLNode(node);
          result->addValue(value);
        } catch(std::runtime_error& e) {
          Logger::getInstance()->log(std::string("SeriesReader::readFromXML: Error while reading value: ") + e.what());
        }
      }
      node = node->nextSibling();
    }

    return result;

  }

  //================================================== SeriesWriter
  //#define LOG_TIMING

  template<class T>
  bool SeriesWriter<T>::writeToXML(const Series<T>& _series, const std::string& _path) {
    AutoPtr<Document> pDoc = new Document;

#ifdef LOG_TIMING
    std::stringstream logSStream;
#endif

#ifdef LOG_TIMING
    Timestamp buildingXML;
#endif
    AutoPtr<ProcessingInstruction> pXMLHeader = pDoc->createProcessingInstruction("xml", "version='1.0' encoding='utf-8'");
    pDoc->appendChild(pXMLHeader);
    AutoPtr<ProcessingInstruction> pProcessing = pDoc->createProcessingInstruction("xml-stylesheet", "href='value_graph.xslt' type='text/xsl'");
    pDoc->appendChild(pProcessing);
    // metering node
    AutoPtr<Element> pRoot = pDoc->createElement("metering");
    pRoot->setAttribute("version", intToString(SeriesXMLFileVersion));
    pDoc->appendChild(pRoot);
    // metering/config
    AutoPtr<Element> pConfig = pDoc->createElement("config");
    pRoot->appendChild(pConfig);

    {
        AutoPtr<Element> elem = pDoc->createElement("type");
        std::string stype;
        if (_series.getType() == kAverage)
          stype = "average";
        else if (_series.getType() == kMaximum)
          stype = "maximum";
        else if (_series.getType() == kAbsolute)
          stype = "absolute";
        else
          stype = "undefined";
        AutoPtr<Text> txt = pDoc->createTextNode(stype);
        elem->appendChild(txt);
        pConfig->appendChild(elem);
    }
    if(!_series.getComment().empty()) {
      AutoPtr<Element> elem = pDoc->createElement("comment");
      AutoPtr<Text> txt = pDoc->createTextNode(_series.getComment());
      elem->appendChild(txt);
      pConfig->appendChild(elem);
    }
    if(_series.getFromDSID() != NullDSID) {
      AutoPtr<Element> elem = pDoc->createElement("from_dsid");
      AutoPtr<Text> txt = pDoc->createTextNode(_series.getFromDSID().toString());
      elem->appendChild(txt);
      pConfig->appendChild(elem);
    }
    if(!_series.getUnit().empty()) {
      AutoPtr<Element> elem = pDoc->createElement("unit");
      AutoPtr<Text> txt = pDoc->createTextNode(_series.getUnit());
      elem->appendChild(txt);
      pConfig->appendChild(elem);
    }

    // metering/values
    AutoPtr<Element> pValues = pDoc->createElement("values");
    pValues->setAttribute("resolution", intToString(_series.getResolution()));
    pValues->setAttribute("numberOfValues", intToString(_series.getNumberOfValues()));
    pRoot->appendChild(pValues);

    const std::deque<T>& values = _series.getValues();
    for(typename std::deque<T>::const_reverse_iterator iValue = values.rbegin(), e = values.rend();
        iValue != e; ++iValue)
    {
      AutoPtr<Element> elem = pDoc->createElement("value");
      iValue->writeToXMLNode(pDoc, elem);
      pValues->appendChild(elem);
    }
#ifdef LOG_TIMING
    logSStream << "building xml: " << Timestamp().getDifference(buildingXML);
    Logger::getInstance()->log(logSStream.str());
    logSStream.str("");
#endif

#ifdef LOG_TIMING
    Timestamp writingXML;
#endif
    // write it to a temporary site first
    std::string tmpOut = _path + ".tmp";
    std::ofstream ofs(tmpOut.c_str() );

    if(ofs) {
      DOMWriter writer;
      writer.setNewLine("\n");
      writer.setOptions(XMLWriter::PRETTY_PRINT);

      // write output to std::stringstream first...
      std::stringstream sstream;
      writer.writeNode(sstream, pDoc);
      std::string content = sstream.str();
      // ...and write the whole thing in one operation
      ofs.write(content.c_str(), content.size());

      ofs.close();

      syncFile(tmpOut);

#ifdef LOG_TIMING
      logSStream << "writing xml: " << Timestamp().getDifference(writingXML);
      Logger::getInstance()->log(logSStream.str());
      logSStream.str("");
#endif

#ifdef LOG_TIMING
      Timestamp renaming;
#endif
      // move it to the desired location
      rename(tmpOut.c_str(), _path.c_str());
#ifdef LOG_TIMING
      logSStream << "renaming: " << Timestamp().getDifference(renaming) << std::endl;
      Logger::getInstance()->log(logSStream.str());
      logSStream.str("");
#endif
    } else {
      Logger::getInstance()->log("Could not open file '" + tmpOut + "' for writing", lsFatal);
    }

    return true;

  }

// #undef LOG_TIMING

  //================================================== Explicit instantiations
  template class SeriesReader<AdderValue>;
  template class SeriesWriter<AdderValue>;
  template class SeriesReader<CurrentValue>;
  template class SeriesWriter<CurrentValue>;

} // namespace dss
