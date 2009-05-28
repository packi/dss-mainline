/*
 * seriespersistence.cpp
 *
 *  Created on: Jan 30, 2009
 *      Author: patrick
 */

#include "seriespersistence.h"

#include "core/xmlwrapper.h"
#include "core/logger.h"
#include "core/foreach.h"

#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

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
  Series<T>* SeriesReader<T>::ReadFromXML(const string& _fileName) {
    Timestamp parsing;

    std::ifstream inFile(_fileName.c_str());

    InputSource input(inFile);
    Series<T>* result = NULL;
    DOMParser parser;
    AutoPtr<Document> pDoc = parser.parse(&input);
    Element* rootNode = pDoc->documentElement();
    if(rootNode->localName() != "metering") {
      Logger::GetInstance()->Log("SeriesReader::ReadFromXML: root node must be named metering, got: '" + rootNode->localName() + "'");
      return NULL;
    }
    if(!rootNode->hasAttribute("version")) {
      Logger::GetInstance()->Log("SeriesReader::ReadFromXML: missing version attribute", lsError);
      return NULL;
    }

    if(StrToIntDef(rootNode->getAttribute("version"),-1) != SeriesXMLFileVersion) {
      Logger::GetInstance()->Log(string("SeriesReader::ReadFromXML: Version mismatch, expected ") + IntToString(SeriesXMLFileVersion) + " got " + rootNode->getAttribute("version"),
                                 lsError);
      return NULL;
    }

    Element* valuesNode = rootNode->getChildElement("values");
    int numberOfValues = StrToInt(valuesNode->getAttribute("numberOfValues"));
    int resolution = StrToInt(valuesNode->getAttribute("resolution"));

    result = new Series<T>(resolution, numberOfValues);

    Node* node = valuesNode->firstChild();
    while(node != NULL) {
      if(node->nodeName() == "value") {
        try {
          T value(0);
          value.ReadFromXMLNode(node);
          result->AddValue(value);
        } catch(runtime_error& e) {
          Logger::GetInstance()->Log(string("SeriesReader::ReadFromXML: Error while reading value: ") + e.what());
        }
      }
      node = node->nextSibling();
    }

    Element* configElem = rootNode->getChildElement("config");
    if(configElem != NULL) {
      Element* elem = configElem->getChildElement("comment");
      if(elem != NULL && elem->hasChildNodes()) {
        result->SetComment(elem->firstChild()->getNodeValue());
      }

      elem = configElem->getChildElement("from_dsid");
      if(elem != NULL && elem->hasChildNodes()) {
        result->SetFromDSID(dsid_t::FromString(elem->firstChild()->getNodeValue()));
      }

      elem = configElem->getChildElement("unit");
      if(elem != NULL && elem->hasChildNodes()) {
        result->SetUnit(elem->firstChild()->getNodeValue());
      }
    }
    return result;

  }

  //================================================== SeriesWriter

  template<class T>
  bool SeriesWriter<T>::WriteToXML(const Series<T>& _series, const string& _path) {
    AutoPtr<Document> pDoc = new Document;

    Timestamp buildingXML;
    AutoPtr<ProcessingInstruction> pXMLHeader = pDoc->createProcessingInstruction("xml", "version='1.0' encoding='utf-8'");
    pDoc->appendChild(pXMLHeader);
    AutoPtr<ProcessingInstruction> pProcessing = pDoc->createProcessingInstruction("xml-stylesheet", "href='value_graph.xslt' type='text/xsl'");
    pDoc->appendChild(pProcessing);
    // metering node
    AutoPtr<Element> pRoot = pDoc->createElement("metering");
    pRoot->setAttribute("version", IntToString(SeriesXMLFileVersion));
    pDoc->appendChild(pRoot);
    // metering/config
    AutoPtr<Element> pConfig = pDoc->createElement("config");
    pRoot->appendChild(pConfig);

    if(!_series.GetComment().empty()) {
      AutoPtr<Element> elem = pDoc->createElement("comment");
      AutoPtr<Text> txt = pDoc->createTextNode(_series.GetComment());
      elem->appendChild(txt);
      pConfig->appendChild(elem);
    }
    if(_series.GetFromDSID() != NullDSID) {
      AutoPtr<Element> elem = pDoc->createElement("from_dsid");
      AutoPtr<Text> txt = pDoc->createTextNode(_series.GetFromDSID().ToString());
      elem->appendChild(txt);
      pConfig->appendChild(elem);
    }
    if(!_series.GetUnit().empty()) {
      AutoPtr<Element> elem = pDoc->createElement("unit");
      AutoPtr<Text> txt = pDoc->createTextNode(_series.GetUnit());
      elem->appendChild(txt);
      pConfig->appendChild(elem);
    }
    
    for(HashMapConstStringString::const_iterator iProperty = _series.GetProperties().GetContainer().begin(),
    	end = _series.GetProperties().GetContainer().end(); iProperty != end; ++iProperty)
    {
      AutoPtr<Element> elem = pDoc->createElement(iProperty->first);
      AutoPtr<Text> txt = pDoc->createTextNode(iProperty->second);
      elem->appendChild(txt);
      pConfig->appendChild(elem);
    }
    

    // metering/values
    AutoPtr<Element> pValues = pDoc->createElement("values");
    pValues->setAttribute("resolution", IntToString(_series.GetResolution()));
    pValues->setAttribute("numberOfValues", IntToString(_series.GetNumberOfValues()));
    pRoot->appendChild(pValues);

    const std::deque<T>& values = _series.GetValues();
    for(typename std::deque<T>::const_reverse_iterator iValue = values.rbegin(), e = values.rend();
        iValue != e; ++iValue)
    {
      AutoPtr<Element> elem = pDoc->createElement("value");
      iValue->WriteToXMLNode(pDoc, elem);
      pValues->appendChild(elem);
    }
    cout << "building xml: " << Timestamp().GetDifference(buildingXML) << endl;


    Timestamp writingXML;
    // write it to a temporary site first
    string tmpOut = _path + ".tmp";
    std::ofstream ofs(tmpOut.c_str() );

    if(ofs) {
      DOMWriter writer;
      writer.setNewLine("\n");
      writer.setOptions(XMLWriter::PRETTY_PRINT);

      // write output to stringstream first...
      std::stringstream sstream;
      writer.writeNode(sstream, pDoc);
      string content = sstream.str();
      // ...and write the whole thing in one operation
      ofs.write(content.c_str(), content.size());

      ofs.close();

      cout << "writing xml: " << Timestamp().GetDifference(writingXML) << endl;

      Timestamp renaming;
      // move it to the desired location
      rename(tmpOut.c_str(), _path.c_str());
      cout << "renaming: " << Timestamp().GetDifference(renaming) << endl;
    } else {
      Logger::GetInstance()->Log("Could not open file for writing");
    }

    return true;

  }

  //================================================== Explicit instantiations
  template class SeriesReader<AdderValue>;
  template class SeriesWriter<AdderValue>;
  template class SeriesReader<CurrentValue>;
  template class SeriesWriter<CurrentValue>;

} // namespace dss
