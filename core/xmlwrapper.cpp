/*
 *  xmlwrapper.cpp
 *  dSS
 *
 *  Created by Patrick Stählin on 4/3/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include "xmlwrapper.h"

namespace dss {
  //============================================= XMLNode
  
  XMLNode::XMLNode() 
  {
    Initialize();
  } // ctor()
  
  XMLNode::XMLNode(const XMLNode& _other) 
  {
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
    m_Attributes = _other.m_Attributes;
    m_AttributesRead = _other.m_AttributesRead;
  } // CopyFrom
  
  const wstring XMLNode::GetName() {
    AssertHasNode("Can't get name without node");
    const char* content = (const char*)m_pNode->name; 
    return FromUTF8(content, strlen(content));
  }
  
  const wstring XMLNode::GetContent() {
    AssertHasNode("Can't get content without node");
    const char* content = (const char*)m_pNode->content;
    return FromUTF8(content, strlen(content));
  }
  
  void XMLNode::AssertHasNode(const string& _reason) {
    if(m_pNode == NULL) {
      throw new XMLException(_reason);
    }
  }
  
  XMLNodeList& XMLNode::GetChildren() {
    AssertHasNode("Can't return children without node");
    
    if(!m_ChildrenRead) {
      xmlNode* child = m_pNode->children;
      while(child != NULL) {
        XMLNode childObj(child);
        m_Children.push_back(childObj);
        child = child->next;
      }
      m_ChildrenRead = true;
    }
    return m_Children;
  }
  
  XMLNode& XMLNode::GetChildByName(const wstring& _name) {
    XMLNodeList& children = GetChildren();
    for(XMLNodeList::iterator it = children.begin(); it != children.end(); ++it) {
      if(it->GetName() == _name) {
        return *it;
      }
    }
    throw new XMLException("Could not find node");
  } // GetChildByName
  
  HashMapConstWStringWString& XMLNode::GetAttributes() {
    AssertHasNode("Can't return children without node");
    
    if(!m_AttributesRead) {
      xmlAttr* currAttr = m_pNode->properties;
      while(currAttr != NULL) {
        if(currAttr->type == XML_ATTRIBUTE_NODE) {
          const char* name = (const char*)currAttr->name;
          const char* value = (const char*)currAttr->children->content;
          
          m_Attributes[FromUTF8(name, strlen(name))] = FromUTF8(value, strlen(value));
        }
        currAttr = currAttr->next;
      }
      m_AttributesRead = true;
    }
    return m_Attributes;
  }
  
  //============================================= XMLDocumentFileReader
  
  XMLDocument::XMLDocument(xmlDoc* _pDocument) 
  : ResourceHolder<xmlDoc>(_pDocument),
  m_OwnsDocument(true)
  {
    if(m_Resource != NULL) {
      m_RootNode = XMLNode(xmlDocGetRootElement(m_Resource));
    }
  } // ctor(xmlDoc*)
  
  XMLDocument::XMLDocument(XMLDocument& _other) 
  : ResourceHolder<xmlDoc>(_other),
  m_RootNode(_other.m_RootNode),
  m_OwnsDocument(false)
  {
  } // ctor(copy)
  
  XMLDocument::~XMLDocument() {
    if(m_Resource != NULL) {
      xmlFreeDoc(m_Resource);
      m_Resource = NULL;
    }
  }
  
  XMLNode& XMLDocument::GetRootNode() {
    return m_RootNode;
  } // GetRootNode
  
  
  //============================================= XMLDocumentFileReader
  
  XMLDocumentFileReader::XMLDocumentFileReader(const wstring& _fileURI) 
  : m_URI(_fileURI)
  {    
  } // ctor
  
  XMLDocumentReader::~XMLDocumentReader() {
    m_Document.FreeDocument();
  }
  
  XMLDocument& XMLDocumentFileReader::GetDocument() {
    const string fileName = ToUTF8(m_URI.c_str(), m_URI.size());
    
    xmlDoc* doc = xmlParseFile(fileName.c_str());
    if(doc == NULL) {
      throw new XMLException(string("Could not parse file: ") + fileName);
    }
    
    XMLDocument docObj(doc);
    m_Document = docObj;
    return m_Document;
  }
  
}