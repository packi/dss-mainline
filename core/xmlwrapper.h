/*
 *  xmlwrapper.h
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

#ifndef _XML_WRAPPER_H_INCLUDED
#define _XML_WRAPPER_H_INCLUDED

#include "base.h"

#include <libxml/tree.h>

namespace dss {
  
  class XMLNode;
  
  typedef vector<XMLNode> XMLNodeList;
  
  class XMLNode {
  private:
    bool m_ChildrenRead;
    bool m_AttributesRead;
  protected:
    xmlNode* m_pNode;
    XMLNodeList m_Children;
    HashMapConstStringString m_Attributes;
    
    void AssertHasNode(const string& _reason);
    void Initialize();
    void CopyFrom(const XMLNode& _other);
  public:
    XMLNode();
    XMLNode(const XMLNode& _other);
    XMLNode(xmlNode* _node);
    
    const string GetName();
    const string GetContent();
    
    void SetContent(const string& _value);
    void SetName(const string& _value);
    
    XMLNode& AddTextNode();
    XMLNode& AddChildNode(const string& _name, const string& _content = "");
    
    XMLNode& GetChildByName(const string& _name);
    
    XMLNodeList& GetChildren();
    HashMapConstStringString& GetAttributes();
  }; // XMLNode
  
  class XMLDocument : ResourceHolder<xmlDoc> {
  private:
    XMLNode m_RootNode;
  public:
    explicit XMLDocument(xmlDoc* _pDocument = NULL);
    XMLDocument(XMLDocument& _other);
    ~XMLDocument();
    
    void SaveToFile(const wstring& _fileName);
    
    XMLNode& GetRootNode();
  }; // XMLDocument
  
  class XMLDocumentReader {
  protected:
    XMLDocument m_Document;
  public:
    virtual ~XMLDocumentReader();
    virtual XMLDocument& GetDocument() = 0;
  }; // XMLReader
  
  class XMLDocumentFileReader : public XMLDocumentReader {
  private:
    const wstring m_URI;
  public:
    XMLDocumentFileReader(const wstring& _uri);
    XMLDocumentFileReader(const string& _uri);
    virtual ~XMLDocumentFileReader() {};
    
    virtual XMLDocument& GetDocument();
  };
  
  class XMLDocumentMemoryReader : public XMLDocumentReader {
  private:
    const string m_XMLData;
  public:
    XMLDocumentMemoryReader(const char* _xmlData);
    virtual ~XMLDocumentMemoryReader() {};
    virtual XMLDocument& GetDocument();
  }; // XMLDocumentMemoryReaders
  
  class XMLException : public DSSException {
  public:
    XMLException( const string& _message ) 
    : DSSException(_message)
    { }
  }; // XMLException
  
  
}

#endif
