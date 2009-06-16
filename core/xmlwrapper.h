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

#ifndef USE_LIBXML
  #include "xml/list.h"
  #include "xml/xmlparser.h"
#else
#include <libxml/tree.h>
#endif

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

    void assertHasNode(const string& _reason);
    void initialize();
    void copyFrom(const XMLNode& _other);
  public:
    XMLNode();
    XMLNode(const XMLNode& _other);
    XMLNode(xmlNode* _node);
    XMLNode& operator=(const XMLNode& _rhs) {
      copyFrom(_rhs);
      return *this;
    }

    const string getName();
    const string getContent();

    void setContent(const string& _value);
    void setName(const string& _value);

    XMLNode& addTextNode();
    XMLNode& addChildNode(const string& _name, const string& _content = "");

    XMLNode& getChildByName(const string& _name);
    bool hasChildWithName(const string& _name);

    XMLNodeList& getChildren();
    HashMapConstStringString& getAttributes();
  }; // XMLNode

  class XMLDocument : ResourceHolder<xmlDoc> {
  private:
    XMLNode m_RootNode;
  public:
    explicit XMLDocument(xmlDoc* _pDocument = NULL);
    XMLDocument(XMLDocument& _other);
    ~XMLDocument();

    void saveToFile(const string& _fileName);

    XMLNode& getRootNode();
  }; // XMLDocument

  class XMLDocumentReader {
  protected:
    XMLDocument m_Document;
  public:
    virtual ~XMLDocumentReader();
    virtual XMLDocument& getDocument() = 0;
  }; // XMLReader

  class XMLDocumentFileReader : public XMLDocumentReader {
  private:
    const string m_URI;
  public:
    XMLDocumentFileReader(const string& _uri);
    virtual ~XMLDocumentFileReader() {}

    virtual XMLDocument& getDocument();
  };

  class XMLDocumentMemoryReader : public XMLDocumentReader {
  private:
    const string m_XMLData;
  public:
    XMLDocumentMemoryReader(const char* _xmlData);
    virtual ~XMLDocumentMemoryReader() {}
    virtual XMLDocument& getDocument();
  }; // XMLDocumentMemoryReaders

  class XMLException : public DSSException {
  public:
    XMLException( const string& _message )
    : DSSException(_message)
    { }
  }; // XMLException


}

#endif
