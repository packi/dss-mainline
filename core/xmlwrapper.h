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
