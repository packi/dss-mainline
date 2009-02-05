/*
 * seriespersistence.cpp
 *
 *  Created on: Jan 30, 2009
 *      Author: patrick
 */

#include "seriespersistence.h"

#include "core/xmlwrapper.h"
#include "core/logger.h"

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

    XMLNode rootNode = reader.GetDocument().GetRootNode();
    if(rootNode.GetName() == "values") {
      if(rootNode.GetAttributes()["version"] != IntToString(SeriesXMLFileVersion)) {
        Logger::GetInstance()->Log(string("SeriesReader::ReadFromXML: Version mismatch, expected ") + IntToString(SeriesXMLFileVersion) + " got " + rootNode.GetAttributes()["version"],
                                   lsError);
        return NULL;
      }
      int numberOfValues = StrToIntDef(rootNode.GetAttributes()["numberOfValues"], -1);
      if(numberOfValues == -1) {
        Logger::GetInstance()->Log(string("SeriesReader::ReadFromXML: numberOfValues attribute missing or invalid: ") + rootNode.GetAttributes()["numberOfValues"],
                                   lsError);
        return NULL;
      }
      int resolution = StrToIntDef(rootNode.GetAttributes()["resolution"], -1);
      if(resolution == -1) {
        Logger::GetInstance()->Log(string("SeriesReader::ReadFromXML: resolution attribute missing or invalid: ") + rootNode.GetAttributes()["numberOfValues"],
                                   lsError);
        return NULL;
      }

      Series<T>* result = new Series<T>(resolution, numberOfValues);
      XMLNodeList& values = rootNode.GetChildren();
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
    AutoPtr<Element> pRoot = pDoc->createElement("values");
    pRoot->setAttribute("version", IntToString(SeriesXMLFileVersion));
    pDoc->appendChild(pRoot);

    const std::deque<T> values = _series.GetValues();
    for(typename std::deque<T>::const_iterator iValue = values.begin(), e = values.end();
        iValue != e; ++iValue)
    {
      AutoPtr<Element> elem = pDoc->createElement("value");
      iValue->WriteToXMLNode(elem);
      pRoot->appendChild(elem);
    }

    std::ofstream ofs(_path.c_str() );

    if(ofs) {
      DOMWriter writer;
      writer.setNewLine("\n");
      writer.setOptions(XMLWriter::PRETTY_PRINT);
      writer.writeNode(ofs, pDoc);

      ofs.close();
    } else {
      Logger::GetInstance()->Log("Could not open file for writing");
    }

    return true;

  }

  //================================================== Explicit instantiations
  template class SeriesReader<AdderValue>;
  template class SeriesWriter<AdderValue>;

} // namespace dss
