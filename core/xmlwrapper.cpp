/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland
    Copyright (c) 2009 futureLAB AG, Winterthur, Switzerland

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

#include "xmlwrapper.h"

#include <fstream>

#include <boost/filesystem.hpp>

namespace dss {
  //============================================= XMLNode

  XMLNode::XMLNode()
  {
    initialize();
  } // ctor()

  XMLNode::XMLNode(const XMLNode& _other)
  {
    initialize();
    copyFrom(_other);
  } // ctor(copy)

  XMLNode::XMLNode(xmlNode* _node)
  {
    initialize();
    m_pNode = _node;
    assertHasNode("XMLNode::ctor(xmlNode*): Node must not be NULL");
  } // ctor(xmlNode*)

  void XMLNode::initialize() {
    m_pNode = NULL;
    m_AttributesRead = false;
    m_ChildrenRead = false;
  } // initialize

  void XMLNode::copyFrom(const XMLNode& _other) {
    m_pNode = _other.m_pNode;
    m_Children = _other.m_Children;
    m_ChildrenRead = _other.m_ChildrenRead;
    if(_other.m_Attributes.size() > 0) {
      m_Attributes = _other.m_Attributes;
    } else {
      m_Attributes.clear();
    }
    m_AttributesRead = _other.m_AttributesRead;
  } // copyFrom

  const std::string XMLNode::getName() {
    assertHasNode("Can't get name without node");
#ifdef USE_LIBXML
    const char* name = (const char*)m_pNode->name;
#else
    const char* name = m_pNode->pName;
#endif
    return name;
  }

  const std::string XMLNode::getContent() {
    assertHasNode("Can't get content without node");
#ifdef USE_LIBXML
    const char* content = (const char*)m_pNode->content;
#else
    const char* content = (const char*)m_pNode->pValue;
#endif
    return content;
  }

  void XMLNode::assertHasNode(const std::string& _reason) {
    if(m_pNode == NULL) {
      throw XMLException(_reason);
    }
  }

  XMLNodeList& XMLNode::getChildren() {
    assertHasNode("Can't return children without node");

    if(!m_ChildrenRead) {
#ifdef USE_LIBXML
      xmlNode* child = m_pNode->children;
      while(child != NULL) {
        XMLNode childObj(child);
        m_Children.push_back(childObj);
        child = child->next;
      }
#else
      lstSeek(m_pNode->subElementList, 0);
      PElement elem = static_cast<PElement>(lstGet(m_pNode->subElementList));
      while(elem != NULL) {
        XMLNode childObj(elem);
        m_Children.push_back(childObj);

        lstNext(m_pNode->subElementList);
        elem = static_cast<PElement>(lstGet(m_pNode->subElementList));
      }
#endif
      m_ChildrenRead = true;
    }
    return m_Children;
  }

  XMLNode& XMLNode::getChildByName(const std::string& _name) {
    XMLNodeList& children = getChildren();
    for(XMLNodeList::iterator it = children.begin(); it != children.end(); ++it) {
      if(it->getName() == _name) {
        return *it;
      }
    }
    throw XMLException("Could not find node");
  } // getChildByName

  bool XMLNode::hasChildWithName(const std::string& _name) {
    XMLNodeList& children = getChildren();
    for(XMLNodeList::iterator it = children.begin(); it != children.end(); ++it) {
      if(it->getName() == _name) {
        return true;
      }
    }
    return false;
  } // hasChildWithName

  XMLNode& XMLNode::addChildNode(const std::string& _name, const std::string& _content) {
    assertHasNode("Need a node to append a child to");

    if(_name.size() == 0) {
      throw XMLException("XMLNode::addChildNode: parameter _name must not be empty");
    }
   /* XMLNode result(xmlNewChild(m_pNode, NULL, _name.c_str(), _content.c_str()));
    m_Children.push_back(result);
    return m_Children[m_Children.size()];
    */
    throw XMLException("XMLNode::addChildNode: parameter _name must not be empty");
  } // addChildNode

  HashMapConstStringString& XMLNode::getAttributes() {
    assertHasNode("Can't return children without node");

    if(!m_AttributesRead) {
#ifdef USE_LIBXML
      xmlAttr* currAttr = m_pNode->properties;
      while(currAttr != NULL) {
        if(currAttr->type == XML_ATTRIBUTE_NODE) {
          const char* name = (const char*)currAttr->name;
          const char* value = (const char*)currAttr->children->content;

          m_Attributes[name] = value;
        }
        currAttr = currAttr->next;
      }
#else
      lstSeek(m_pNode->attributeList, 0);
      PAttribute attr = static_cast<PAttribute>(lstGet(m_pNode->attributeList));
      while(attr != NULL) {
        const char* name = attr->pName;
        const char* value = attr->pValue;

        m_Attributes[name] = value;
        lstNext(m_pNode->attributeList);
        attr = static_cast<PAttribute>(lstGet(m_pNode->attributeList));
      }
#endif
      m_AttributesRead = true;
    }
    return m_Attributes;
  }

  //============================================= XMLDocument

  XMLDocument::XMLDocument(xmlDoc* _pDocument)
  : ResourceHolder<xmlDoc>(_pDocument)
  {
    if(m_Resource != NULL) {
#ifdef USE_LIBXML
      m_RootNode = XMLNode(xmlDocGetRootElement(m_Resource));
#else
      lstSeek(_pDocument, 0);
      m_RootNode = XMLNode(static_cast<PElement>(lstGet(_pDocument)));
#endif
    }
  } // ctor(xmlDoc*)

  XMLDocument::XMLDocument(XMLDocument& _other)
  : ResourceHolder<xmlDoc>(_other),
  m_RootNode(_other.m_RootNode)
  {
  } // ctor(copy)

  XMLDocument::~XMLDocument() {
    if(m_Resource != NULL) {
#ifdef USE_LIBXML
      xmlFreeDoc(m_Resource);
#else
      lstDeinit(m_Resource);
#endif
      m_Resource = NULL;
    }
  }

  XMLNode& XMLDocument::getRootNode() {
    return m_RootNode;
  } // getRootNode


  void XMLDocument::saveToFile(const std::string& _fileName) {
    FILE* out = fopen(_fileName.c_str(), "w");
    if(out == NULL) {
      throw XMLException(std::string("XMLDocumen::saveToFile: Could not open file ") + _fileName);
    }
#ifdef USE_LIBXML
    if(xmlDocDump(out, m_Resource) < 0) {
      throw XMLException(std::string("XMLDocumen::saveToFile: xmlDocDump failed for file:") + _fileName);
    }
#endif
  }

  //============================================= XMLDocumentFileReader

  XMLDocumentFileReader::XMLDocumentFileReader(const std::string& _uri)
  : m_URI(_uri)
  {
  }

  XMLDocumentReader::~XMLDocumentReader() {
  }

  XMLDocument& XMLDocumentFileReader::getDocument() {
    if(!boost::filesystem::exists(m_URI.c_str())) {
      throw XMLException(std::string("XMLDocumentFileReader::getDocument: File '") + m_URI + "' does not exist");
    }

#ifdef USE_LIBXML
    xmlDoc* doc = xmlParseFile(m_URI.c_str());
    if(doc == NULL) {
      throw XMLException(std::string("Could not parse file: ") + m_URI);
    }

    XMLDocument docObj(doc);
    m_Document = docObj;
#else
    ifstream f(m_URI.c_str());
    std::string str((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());

    PScanner scanner;
    scanner = scnInit();
    scanner->buf = str.c_str();
    scanner->bufLength = str.size();
    xmlDoc* doc = xmlParseXML(scanner);
    XMLDocument docObj(doc);
    m_Document = docObj;
#endif
    return m_Document;
  }

}
