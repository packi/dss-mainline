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


#include "restfulapiwriter.h"
#include "core/foreach.h"

#include <fstream>

#include <Poco/DOM/Attr.h>
#include <Poco/DOM/Text.h>
#include <Poco/DOM/ProcessingInstruction.h>
#include <Poco/DOM/DOMWriter.h>
#include <Poco/XML/XMLWriter.h>

using Poco::XML::Document;
using Poco::XML::Element;
using Poco::XML::Attr;
using Poco::XML::Text;
using Poco::XML::ProcessingInstruction;
using Poco::XML::AutoPtr;
using Poco::XML::DOMWriter;
using Poco::XML::XMLWriter;

namespace dss {

  void RestfulAPIWriter::writeToXML(const RestfulAPI& api, const std::string& _location) {
    AutoPtr<Document> pDoc = new Document;

    AutoPtr<ProcessingInstruction> pXMLHeader = pDoc->createProcessingInstruction("xml", "version='1.0' encoding='utf-8'");
    pDoc->appendChild(pXMLHeader);
    AutoPtr<ProcessingInstruction> pXMLStylesheet = pDoc->createProcessingInstruction("xml-stylesheet", "type='text/xml' href='json_api.xslt'");
    pDoc->appendChild(pXMLStylesheet);

    AutoPtr<Element> pRoot = pDoc->createElement("api");
    pDoc->appendChild(pRoot);
    AutoPtr<Element> pClasses = pDoc->createElement("classes");
    pRoot->appendChild(pClasses);

    foreach(const RestfulClass& cls, api.getClasses()) {
      AutoPtr<Element> pClass = pDoc->createElement("class");
      AutoPtr<Element> pClassNameNode = pDoc->createElement("name");
      pClass->appendChild(pClassNameNode);
      AutoPtr<Text> pClassNameTextNode = pDoc->createTextNode(cls.getName());
      pClassNameNode->appendChild(pClassNameTextNode);

      AutoPtr<Element> pInstanceParamNode = pDoc->createElement("instanceParameter");
      pClass->appendChild(pInstanceParamNode);
      foreach(const RestfulParameter& instanceParam, cls.getInstanceParameter()) {
        AutoPtr<Element> pParameter = writeToXML(instanceParam, pDoc);
        pInstanceParamNode->appendChild(pParameter);
      }


      if((!cls.getDocumentationShort().empty()) || (!cls.getDocumentationLong().empty())) {
        AutoPtr<Element> pDocumentation = pDoc->createElement("documentation");
        if(!cls.getDocumentationShort().empty()) {
          AutoPtr<Element> pShortNode = pDoc->createElement("short");
          AutoPtr<Text> pShortText = pDoc->createTextNode(cls.getDocumentationShort());
          pShortNode->appendChild(pShortText);
          pDocumentation->appendChild(pShortNode);
        }
        if(!cls.getDocumentationLong().empty()) {
          AutoPtr<Element> pLongNode = pDoc->createElement("long");
          AutoPtr<Text> pLongText = pDoc->createTextNode(cls.getDocumentationLong());
          pLongNode->appendChild(pLongText);
          pDocumentation->appendChild(pLongNode);
        }
        pClass->appendChild(pDocumentation);
      }


      AutoPtr<Element> pMethods = pDoc->createElement("methods");
      foreach(const RestfulMethod& method, cls.getMethods()) {
        AutoPtr<Element> pMethod = pDoc->createElement("method");
        AutoPtr<Element> pMethodNameNode = pDoc->createElement("name");
        pMethod->appendChild(pMethodNameNode);
        AutoPtr<Text> pMethodNameTextNode = pDoc->createTextNode(method.getName());
        pMethodNameNode->appendChild(pMethodNameTextNode);

        if((!method.getDocumentationShort().empty()) || (!method.getDocumentationLong().empty())) {
          AutoPtr<Element> pDocumentation = pDoc->createElement("documentation");
          if(!method.getDocumentationShort().empty()) {
            AutoPtr<Element> pShortNode = pDoc->createElement("short");
            AutoPtr<Text> pShortText = pDoc->createTextNode(method.getDocumentationShort());
            pShortNode->appendChild(pShortText);
            pDocumentation->appendChild(pShortNode);
          }
          if(!method.getDocumentationLong().empty()) {
            AutoPtr<Element> pLongNode = pDoc->createElement("long");
            AutoPtr<Text> pLongText = pDoc->createTextNode(method.getDocumentationLong());
            pLongNode->appendChild(pLongText);
            pDocumentation->appendChild(pLongNode);
          }
          pMethod->appendChild(pDocumentation);
        }

        AutoPtr<Element> pParams = pDoc->createElement("parameter");
        foreach(const RestfulParameter& parameter, method.getParameter()) {
          AutoPtr<Element> pParameter = writeToXML(parameter, pDoc);
          pParams->appendChild(pParameter);
        }
        pMethod->appendChild(pParams);

        pMethods->appendChild(pMethod);
      }
      pClass->appendChild(pMethods);

      pClasses->appendChild(pClass);
    }

    std::ofstream ofs(_location.c_str());
    if(ofs) {
      DOMWriter writer;
      writer.setNewLine("\n");
      writer.setOptions(XMLWriter::PRETTY_PRINT);
      writer.writeNode(ofs, pDoc);
      ofs.close();
    }
  } // writeToXML

  Poco::XML::AutoPtr<Poco::XML::Element> RestfulAPIWriter::writeToXML(const RestfulParameter& _parameter, Poco::XML::AutoPtr<Poco::XML::Document>& _document) {
    AutoPtr<Element>  pParameter = _document->createElement("parameter");

    AutoPtr<Element> pParameterNameNode = _document->createElement("name");
    pParameter->appendChild(pParameterNameNode);
    AutoPtr<Text> pParameterNameTextNode = _document->createTextNode(_parameter.getName());
    pParameterNameNode->appendChild(pParameterNameTextNode);

    AutoPtr<Element> pParameterTypeNode = _document->createElement("type");
    pParameter->appendChild(pParameterTypeNode);
    AutoPtr<Text> pParameterTypeTextNode = _document->createTextNode(_parameter.getTypeName());
    pParameterTypeNode->appendChild(pParameterTypeTextNode);

    AutoPtr<Element> pParameterRequiredNode = _document->createElement("required");
    pParameter->appendChild(pParameterRequiredNode);
    AutoPtr<Text> pParameterRequiredTextNode = _document->createTextNode(_parameter.isRequired() ? "true" : "false");
    pParameterRequiredNode->appendChild(pParameterRequiredTextNode);

    return pParameter;
  } // writeToXML


} // namespace dss

