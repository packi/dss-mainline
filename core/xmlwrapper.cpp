/*
 *  xmlwrapper.cpp
 *  dSS
 *
 *  Created by Patrick Stählin on 4/3/08.
 *  Copyright:
 *  (c) 2008 by
 *  futureLAB AG
 *  Schwalmenackerstrasse 4
 *  CH-8400 Winterthur / Schweiz
 *  Alle Rechte vorbehalten.
 *  Jede Art der Vervielfaeltigung, Verbreitung,
 *  Auswertung oder Veraenderung - auch auszugsweise -
 *  ist ohne vorgaengige schriftliche Genehmigung durch
 *  die futureLAB AG untersagt.
 *
 * Last change $Date$
 * by $Author$
 */

#include "xmlwrapper.h"

#include <iostream>
#include <fstream>

namespace dss {
  //============================================= XMLNode

  XMLNode::XMLNode()
  {
    Initialize();
  } // ctor()

  XMLNode::XMLNode(const XMLNode& _other)
  {
	Initialize();
    CopyFrom(_other);
  } // ctor(copy)

  XMLNode::XMLNode(xmlNode* _node)
  {
    Initialize();
    m_pNode = _node;
    AssertHasNode("XMLNode::ctor(xmlNode*): Node must not be NULL");
  } // ctor(xmlNode*)

  void XMLNode::Initialize() {
    m_pNode = NULL;
    m_AttributesRead = false;
    m_ChildrenRead = false;
  } // Initialize

  void XMLNode::CopyFrom(const XMLNode& _other) {
    m_pNode = _other.m_pNode;
    m_Children = _other.m_Children;
    m_ChildrenRead = _other.m_ChildrenRead;
    if(_other.m_Attributes.size() > 0) {
      m_Attributes = _other.m_Attributes;
    } else {
      m_Attributes.clear();
    }
    m_AttributesRead = _other.m_AttributesRead;
  } // CopyFrom

  const string XMLNode::GetName() {
    AssertHasNode("Can't get name without node");
#ifdef USE_LIBXML
    const char* name = (const char*)m_pNode->name;
#else
    const char* name = m_pNode->pName;
#endif
    return name;
  }

  const string XMLNode::GetContent() {
    AssertHasNode("Can't get content without node");
#ifdef USE_LIBXML
    const char* content = (const char*)m_pNode->content;
#else
    const char* content = (const char*)m_pNode->pValue;
#endif
    return content;
  }

  void XMLNode::AssertHasNode(const string& _reason) {
    if(m_pNode == NULL) {
      throw XMLException(_reason);
    }
  }

  XMLNodeList& XMLNode::GetChildren() {
    AssertHasNode("Can't return children without node");

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

  XMLNode& XMLNode::GetChildByName(const string& _name) {
    XMLNodeList& children = GetChildren();
    for(XMLNodeList::iterator it = children.begin(); it != children.end(); ++it) {
      if(it->GetName() == _name) {
        return *it;
      }
    }
    throw XMLException("Could not find node");
  } // GetChildByName

  XMLNode& XMLNode::AddChildNode(const string& _name, const string& _content) {
    if(_name.size() == 0) {
      throw XMLException("XMLNode::AddChildNode: parameter _name must not be empty");
    }
    throw XMLException("Not yet implemented");
  } // AddChildNode

  HashMapConstStringString& XMLNode::GetAttributes() {
    AssertHasNode("Can't return children without node");

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

  XMLNode& XMLDocument::GetRootNode() {
    return m_RootNode;
  } // GetRootNode


  void XMLDocument::SaveToFile(const string& _fileName) {
    FILE* out = fopen(_fileName.c_str(), "w");
    if(out == NULL) {
      throw XMLException(string("XMLDocumen::SaveToFile: Could not open file ") + _fileName);
    }
#ifdef USE_LIBXML
    if(xmlDocDump(out, m_Resource) < 0) {
      throw XMLException(string("XMLDocumen::SaveToFile: xmlDocDump failed for file:") + _fileName);
    }
#endif
  }

  //============================================= XMLDocumentFileReader

  XMLDocumentFileReader::XMLDocumentFileReader(const string& _uri)
  : m_URI(_uri)
  {
  }

  XMLDocumentReader::~XMLDocumentReader() {
  }

  XMLDocument& XMLDocumentFileReader::GetDocument() {
    if(!FileExists(m_URI.c_str())) {
      throw XMLException(string("XMLDocumentFileReader::GetDocument: File '") + m_URI + "' does not exist");
    }

#ifdef USE_LIBXML
    xmlDoc* doc = xmlParseFile(m_URI.c_str());
    if(doc == NULL) {
      throw XMLException(string("Could not parse file: ") + m_URI);
    }

    XMLDocument docObj(doc);
    m_Document = docObj;
#else
    ifstream f(m_URI.c_str());
    string str((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());

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
