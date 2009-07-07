/*
 * restfulapiwriter.cpp
 *
 *  Created on: Jul 6, 2009
 *      Author: patrick
 */

#include "restfulapiwriter.h"
#include "core/foreach.h"

#include <fstream>

#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/Attr.h>
#include <Poco/DOM/Text.h>
#include <Poco/DOM/ProcessingInstruction.h>
#include <Poco/DOM/AutoPtr.h>
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

  void RestfulAPIWriter::WriteToXML(const RestfulAPI& api, const std::string& _location) {
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

      AutoPtr<Element> pMethods = pDoc->createElement("methods");
      foreach(const RestfulMethod& method, cls.getMethods()) {
        AutoPtr<Element> pMethod = pDoc->createElement("method");
        AutoPtr<Element> pMethodNameNode = pDoc->createElement("name");
        pMethod->appendChild(pMethodNameNode);
        AutoPtr<Text> pMethodNameTextNode = pDoc->createTextNode(method.getName());
        pMethodNameNode->appendChild(pMethodNameTextNode);

        AutoPtr<Element> pParams = pDoc->createElement("parameter");
        foreach(const RestfulParameter& parameter, method.getParameter()) {
          AutoPtr<Element> pParameter = pDoc->createElement("parameter");

          AutoPtr<Element> pParameterNameNode = pDoc->createElement("name");
          pParameter->appendChild(pParameterNameNode);
          AutoPtr<Text> pParameterNameTextNode = pDoc->createTextNode(parameter.getName());
          pParameterNameNode->appendChild(pParameterNameTextNode);
          
          AutoPtr<Element> pParameterTypeNode = pDoc->createElement("type");
          pParameter->appendChild(pParameterTypeNode);
          AutoPtr<Text> pParameterTypeTextNode = pDoc->createTextNode(parameter.getTypeName());
          pParameterTypeNode->appendChild(pParameterTypeTextNode);

          AutoPtr<Element> pParameterRequiredNode = pDoc->createElement("required");
          pParameter->appendChild(pParameterRequiredNode);
          AutoPtr<Text> pParameterRequiredTextNode = pDoc->createTextNode(parameter.isRequired() ? "true" : "false");
          pParameterRequiredNode->appendChild(pParameterRequiredTextNode);

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
  } // WriteToXML

} // namespace dss

