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
#include <Poco/XML/XMLWriter.h>

#include <iostream>
#include <fstream>


using Poco::XML::Document;
using Poco::XML::Element;
using Poco::XML::Attr;
using Poco::XML::ProcessingInstruction;
using Poco::XML::Text;
using Poco::XML::AutoPtr;
using Poco::XML::DOMWriter;
using Poco::XML::XMLWriter;


namespace dss {

  const int SeriesXMLFileVersion = 1;

  //================================================== SeriesReader

  template<class T>
  Series<T>* SeriesReader<T>::ReadFromXML(const string& _fileName) {
    XMLDocumentFileReader reader(_fileName);

    XMLNode& rootNode = reader.GetDocument().GetRootNode();
    if(rootNode.GetName() == "metering") {
      Series<T>* result = NULL;
      if(rootNode.GetAttributes()["version"] != IntToString(SeriesXMLFileVersion)) {
        Logger::GetInstance()->Log(string("SeriesReader::ReadFromXML: Version mismatch, expected ") + IntToString(SeriesXMLFileVersion) + " got " + rootNode.GetAttributes()["version"],
                                   lsError);
        return NULL;
      }
      XMLNodeList& children = rootNode.GetChildren();
      foreach(XMLNode node, children) {
        if(node.GetName() == "values") {
          int numberOfValues = StrToIntDef(node.GetAttributes()["numberOfValues"], -1);
          if(numberOfValues == -1) {
            Logger::GetInstance()->Log(string("SeriesReader::ReadFromXML: numberOfValues attribute missing or invalid: ") + node.GetAttributes()["numberOfValues"],
                                       lsError);
            return NULL;
          }
          int resolution = StrToIntDef(node.GetAttributes()["resolution"], -1);
          if(resolution == -1) {
            Logger::GetInstance()->Log(string("SeriesReader::ReadFromXML: resolution attribute missing or invalid: ") + node.GetAttributes()["numberOfValues"],
                                       lsError);
            return NULL;
          }

          result = new Series<T>(resolution, numberOfValues);
          XMLNodeList& values = node.GetChildren();
          for(XMLNodeList::iterator iNode = values.begin(), e = values.end();
              iNode != e; ++iNode)
          {
            if(iNode->GetName() == "value") {
              try {
                T value(0);
                value.ReadFromXMLNode(*iNode);
                result->AddValue(value);
              } catch(runtime_error& e) {
                Logger::GetInstance()->Log(string("SeriesReader::ReadFromXML: Error while reading value: ") + e.what());
              }
            }
          }
        }
      }
      foreach(XMLNode node, children) {
        if(node.GetName() == "config") {
          try {
            XMLNode& commentNode = node.GetChildByName("comment");
            if(result != NULL) {
              result->SetComment(commentNode.GetChildren()[0].GetContent());
            }
          } catch(runtime_error&) {
          }
          try {
            XMLNode& dsidNode = node.GetChildByName("from_dsid");
            if(result != NULL) {
              result->SetFromDSID(dsid_t::FromString(dsidNode.GetChildren()[0].GetContent()));
            }
          } catch(runtime_error&) {
          }
          try {
            XMLNode& unitNode = node.GetChildByName("unit");
            if(result != NULL) {
              result->SetUnit(unitNode.GetChildren()[0].GetContent());
            }
          } catch(runtime_error&) {
          }
        }
      }
      return result;
    }
    return NULL;
  }

  //================================================== SeriesWriter

  template<class T>
  bool SeriesWriter<T>::WriteToXML(const Series<T>& _series, const string& _path) {
    AutoPtr<Document> pDoc = new Document;

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
    //pRoot->appendChild(pConfig);
    // metering/values
    AutoPtr<Element> pValues = pDoc->createElement("values");
    pValues->setAttribute("resolution", IntToString(_series.GetResolution()));
    pValues->setAttribute("numberOfValues", IntToString(_series.GetNumberOfValues()));
    pRoot->appendChild(pValues);

    const std::deque<T> values = _series.GetValues();
    for(typename std::deque<T>::const_reverse_iterator iValue = values.rbegin(), e = values.rend();
        iValue != e; ++iValue)
    {
      AutoPtr<Element> elem = pDoc->createElement("value");
      iValue->WriteToXMLNode(pDoc, elem);
      pValues->appendChild(elem);
    }

    // write it to a temporary site first
    string tmpOut = _path + ".tmp";
    std::ofstream ofs(tmpOut.c_str() );

    if(ofs) {
      DOMWriter writer;
      writer.setNewLine("\n");
      writer.setOptions(XMLWriter::PRETTY_PRINT);
      writer.writeNode(ofs, pDoc);

      ofs.close();

      // move it to the desired location
      rename(tmpOut.c_str(), _path.c_str());
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
