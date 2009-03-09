#include "propertysystem.h"

#include "base.h"

#include <iostream>
#include <sstream>
#include <algorithm>
#include <cassert>
#include <cstring>

namespace dss {

  const int PROPERTY_FORMAT_VERSION = 1;
#define XML_ENCODING "utf-8"

  xmlAttr* xmlSearchAttr(xmlNode* _pNode, xmlChar* _name) {
    xmlAttr* curAttr = _pNode->properties;

    while (curAttr != NULL) {
      if (curAttr->type == XML_ATTRIBUTE_NODE) {
        if (strcmp((char*) curAttr->name, (char*) _name) == 0) {
          return curAttr;
        }
      }
      curAttr = curAttr->next;
    };

    return NULL;
  } // xmlSearchAttr

  xmlNode* xmlSearchNode(xmlNode* _pNode, const char* _name, bool _recurse) {
    xmlNode* result = _pNode->children;

    while (result != NULL) {
      if (result->type == XML_ELEMENT_NODE) {
        if (strcmp((char*) result->name, _name) == 0) {
          break;
        } else {
          if (_recurse) {
            xmlNode* propRes = xmlSearchNode(result, _name, true);
            if (propRes != NULL) {
              result = propRes;
              break;
            }
          }
        }
      }
      result = result->next;
    }
    return result;
  } // xmlSearchNode

  //=============================================== PropertySystem

  PropertySystem::PropertySystem() :
    m_RootNode(new PropertyNode(NULL, "/")) {
  } // ctor


  PropertySystem::~PropertySystem() {
  } // dtor


  bool PropertySystem::LoadFromXML(const std::string& _fileName,
                                   PropertyNode* _rootNode) {
    xmlDoc* doc = NULL;
    xmlNode *rootElem = NULL;

    PropertyNode* rootNode = _rootNode;
    if (rootNode == NULL) {
      rootNode = GetRootNode();
    }

    xmlInitParser();

    doc = xmlParseFile(_fileName.c_str());
    if (doc == NULL) {
      cerr << "Error loading properties from \"" << _fileName << "\"" << endl;
      return false;
    }
    rootElem = xmlDocGetRootElement(doc);

    bool versionOK = false;
    if (strcmp((char*) rootElem->name, "properties") == 0) {
      xmlAttr* versionAttr = xmlSearchAttr(rootElem, (xmlChar*) "version");
      if ((versionAttr != NULL) && (versionAttr->children->content != NULL)) {
        if (atoi((char*) versionAttr->children->content)
            == PROPERTY_FORMAT_VERSION) {
          versionOK = true;
          if ((rootElem->children != NULL)) {
            xmlNode* curNode = rootElem->children;
            // search for the first property node and go on from there
            while (curNode != NULL) {
              if ((curNode->type == XML_ELEMENT_NODE)
                  && (strcmp((char*) curNode->name, "property") == 0)) {
                rootNode->LoadFromNode(curNode);
                break;
              }
              curNode = curNode->next;
            }
          }
        }
      }
    }
    if (!versionOK) {
      cerr << "Version mismatch while loading properties from \"" << _fileName
          << "\"" << endl;
    }

    xmlFreeDoc(doc);
    xmlCleanupParser();
    return false;
  } // LoadFromXML


  bool PropertySystem::SaveToXML(const std::string& _fileName, PropertyNode* _rootNode) const {
    int rc;
    xmlTextWriterPtr writer;
    PropertyNode* root = _rootNode;
    if (root == NULL) {
      root = m_RootNode;
    }

    writer = xmlNewTextWriterFilename(_fileName.c_str(), 0);
    if (writer == NULL) {
      return false;
    }

    rc = xmlTextWriterStartDocument(writer, NULL, XML_ENCODING, NULL);
    if(rc < 0) {
      return false;
    }

    rc = xmlTextWriterStartElement(writer, (xmlChar*)"properties");
    if(rc < 0) {
      return false;
    }

    rc = xmlTextWriterWriteFormatAttribute(writer, (xmlChar*)"version", "%d", PROPERTY_FORMAT_VERSION);
    if(rc < 0) {
      return false;
    }

    root->SaveAsXML(writer);

    rc = xmlTextWriterEndElement(writer);
    if(rc < 0) {
      return false;
    }

    rc = xmlTextWriterEndDocument(writer);
    if(rc < 0) {
      return false;
    }

    xmlFreeTextWriter(writer);

    return true;
  } // SaveToXML


  PropertyNode* PropertySystem::GetProperty(const std::string& _propPath) const {
    if(_propPath[ 0 ] != '/') {
      return NULL;
    }
    string propPath = _propPath;
    propPath.erase(0, 1);
    if(propPath.length() == 0) {
      return m_RootNode;
    }
    return m_RootNode->GetProperty(propPath);
  } // GetProperty


  PropertyNode* PropertySystem::CreateProperty(const std::string& _propPath) {
    if(_propPath[ 0 ] != '/') {
      return NULL;
    }
    string propPath = _propPath;
    propPath.erase(0, 1);
    if(propPath.length() == 0) {
      return m_RootNode;
    }
    return m_RootNode->CreateProperty(propPath);
  } // CreateProperty

  int PropertySystem::GetIntValue(const std::string& _propPath) const {
    PropertyNode* prop = GetProperty(_propPath);
    if(prop != NULL) {
      return prop->GetIntegerValue();
    } else {
      return 0;
    }
  } // GetIntValue

  bool PropertySystem::GetBoolValue(const std::string& _propPath) const {
    PropertyNode* prop = GetProperty(_propPath);
    if(prop != NULL) {
      return prop->GetBoolValue();
    } else {
      return false;
    }
  } // GetBoolValue

  std::string PropertySystem::GetStringValue(const std::string& _propPath) const {
    PropertyNode* prop = GetProperty(_propPath);
    if(prop != NULL) {
      return prop->GetStringValue();
    } else {
      return std::string();
    }
  } // GetStringValue

  bool PropertySystem::SetIntValue(const std::string& _propPath, const int _value, bool _mayCreate) {
    if(_mayCreate) {
      PropertyNode* prop = CreateProperty(_propPath);
      prop->SetIntegerValue(_value);
      return true;
    } else {
      PropertyNode* prop = GetProperty(_propPath);
      if(prop != NULL) {
        prop->SetIntegerValue(_value);
        return true;
      } else {
        return false;
      }
    }
  } // SetIntValue

  bool PropertySystem::SetBoolValue(const std::string& _propPath, const bool _value, bool _mayCreate) {
    if(_mayCreate) {
      PropertyNode* prop = CreateProperty(_propPath);
      prop->SetBooleanValue(_value);
      return true;
    } else {
      PropertyNode* prop = GetProperty(_propPath);
      if(prop != NULL) {
        prop->SetBooleanValue(_value);
        return true;
      } else {
        return false;
      }
    }
  } // SetBoolValue

  bool PropertySystem::SetStringValue(const std::string& _propPath, const std::string& _value, bool _mayCreate) {
    if(_mayCreate) {
      PropertyNode* prop = CreateProperty(_propPath);
      prop->SetStringValue(_value);
      return true;
    } else {
      PropertyNode* prop = GetProperty(_propPath);
      if(prop != NULL) {
        prop->SetStringValue(_value);
        return true;
      } else {
        return false;
      }
    }
  } // SetStringValue


  //=============================================== PropertyNode

  PropertyNode::PropertyNode(PropertyNode* _parentNode, const char* _name, int _index)
    : m_ParentNode(_parentNode),
      m_Name(_name),
      m_LinkedToProxy(false),
      m_Index(_index)
  {
    memset(&m_PropVal, '\0', sizeof(aPropertyValue));
    if(_parentNode) {
      _parentNode->AddChild(this);
    }
  } // ctor


  PropertyNode::~PropertyNode() {
    if(m_PropVal.valueType == vTypeString) {
      if(m_PropVal.actualValue.pString != NULL) {
        free(m_PropVal.actualValue.pString);
      }
    }
    for(vector<PropertyNode*>::iterator it = m_ChildNodes.begin();
         it != m_ChildNodes.end();) {
      delete *it;
      it = m_ChildNodes.erase(it);
    }
  } // dtor


  void PropertyNode::AddChild(PropertyNode* _childNode) {
    m_ChildNodes.push_back(_childNode);
  } // AddChild


  const std::string& PropertyNode::GetDisplayName() const {
    if(m_ParentNode->Count(m_Name) > 1) {
      stringstream sstr;
      sstr << GetName() << "[" << m_Index << "]" << endl;
      m_DisplayName = sstr.str();
      return m_DisplayName;
    } else {
      return m_Name;
    }
  } // GetDisplayName


  PropertyNode* PropertyNode::GetProperty(const std::string& _propPath) {
    string propPath = _propPath;
    string propName = _propPath;
    string::size_type slashPos = propPath.find('/');
    if(slashPos != string::npos) {
      propName = propPath.substr(0, slashPos);
      propPath.erase(0, slashPos + 1);
      PropertyNode* child = GetPropertyByName(propName);
      if(child != NULL) {
        return child->GetProperty(propPath);
      } else {
        return NULL;
      }
    } else {
      return GetPropertyByName(propName);
    }
  } // GetProperty


  int PropertyNode::GetAndRemoveIndexFromPropertyName(std::string& _propName) {
    int result = 0;
    string::size_type pos = _propName.find('[');
    if(pos != string::npos) {
      string::size_type end = _propName.find(']');
      string indexAsString = _propName.substr(pos + 1, end - pos - 1);
      _propName.erase(pos, end);
      if(Trim(indexAsString) == "last") {
        result = -1;
      } else {
        result = atoi(indexAsString.c_str());
      }
    }
    return result;
  } // GetAndRemoveIndexFromPropertyName


  PropertyNode* PropertyNode::GetPropertyByName(const std::string& _name) {
    int index = 0;
    string propName = _name;
    index = GetAndRemoveIndexFromPropertyName(propName);

    int curIndex = 0;
    int lastMatch = -1;
    int numItem = 0;
    for(vector<PropertyNode*>::iterator it = m_ChildNodes.begin();
         it != m_ChildNodes.end(); it++) {
      numItem++;
      PropertyNode* cur = *it;
      if(cur->m_Name == propName) {
        if(curIndex == index) {
          return cur;
        }
        curIndex++;
        lastMatch = numItem - 1;
      }
    }
    if(index == -1) {
      if(lastMatch != -1) {
        return m_ChildNodes[ lastMatch ];
      }
    }
    return NULL;
  } // GetPropertyName


  int PropertyNode::Count(const std::string& _propertyName) {
    int result = 0;
    for(vector<PropertyNode*>::iterator it = m_ChildNodes.begin();
         it != m_ChildNodes.end(); it++) {
      PropertyNode* cur = *it;
      if(cur->m_Name == _propertyName) {
        result++;
      }
    }
    return result;
  } // Count


  void PropertyNode::ClearValue() {
    if(m_PropVal.valueType == vTypeString) {
      if(m_PropVal.actualValue.pString != NULL) {
        free(m_PropVal.actualValue.pString);
      }
    }
    memset(&m_PropVal, '\0', sizeof(aPropertyValue));
  } // ClearValue


  void PropertyNode::SetStringValue(const char* _value) {
    ClearValue();
    if(m_LinkedToProxy) {
      m_Proxy.stringProxy->SetValue(_value);
    } else {
      if(_value != NULL) {
        m_PropVal.actualValue.pString = (char*)malloc(strlen(_value) + 1);
        strcpy(m_PropVal.actualValue.pString, _value);
      }
    }
    m_PropVal.valueType = vTypeString;
    NotifyListeners(&PropertyListener::PropertyChanged);
  } // SetStringValue


  void PropertyNode::SetStringValue(const std::string& _value) {
    SetStringValue(_value.c_str());
  } // SetStringValue


  void PropertyNode::SetIntegerValue(const int _value) {
    ClearValue();
    if(m_LinkedToProxy) {
      m_Proxy.intProxy->SetValue(_value);
    } else {
      m_PropVal.actualValue.Integer = _value;
    }
    m_PropVal.valueType = vTypeInteger;
    NotifyListeners(&PropertyListener::PropertyChanged);
  } // SetIntegerValue


  void PropertyNode::SetBooleanValue(const bool _value) {
    ClearValue();
    if(m_LinkedToProxy) {
      m_Proxy.boolProxy->SetValue(_value);
    } else {
      m_PropVal.actualValue.Boolean = _value;
    }
    m_PropVal.valueType = vTypeBoolean;
    NotifyListeners(&PropertyListener::PropertyChanged);
  } // SetBooleanValue


  std::string PropertyNode::GetStringValue() {
    if(m_PropVal.valueType == vTypeString || m_LinkedToProxy) {
      if(m_LinkedToProxy) {
        return m_Proxy.stringProxy->GetValue();
      } else {
        return m_PropVal.actualValue.pString;
      }
    } else {
      cerr << "Property-Type missmatch: " << m_Name << endl;
      throw runtime_error("Property-Type missmatch: " + m_Name);
    }
  } // GetStringValue


  int PropertyNode::GetIntegerValue() {
    if(m_PropVal.valueType == vTypeInteger || m_LinkedToProxy) {
      if(m_LinkedToProxy) {
        return m_Proxy.intProxy->GetValue();
      } else {
        return m_PropVal.actualValue.Integer;
      }
    } else {
      cerr << "Property-Type missmatch: " << m_Name << endl;
      return 0;
    }
  } // GetIntegerValue


  bool PropertyNode::GetBoolValue() {
    if(m_PropVal.valueType == vTypeBoolean || m_LinkedToProxy) {
      if(m_LinkedToProxy) {
        return m_Proxy.boolProxy->GetValue();
      } else {
        return m_PropVal.actualValue.Boolean;
      }
    } else {
      cerr << "Property-Type missmatch: " << m_Name << endl;
      return false;
    }
  } // GetBoolValue

  bool PropertyNode::LinkToProxy(const PropertyProxy<bool>& _proxy) {
    if(m_LinkedToProxy) {
      return false;
    }
    m_Proxy.boolProxy = _proxy.clone();
    m_LinkedToProxy = true;
    m_PropVal.valueType = vTypeBoolean;
    return true;
  } // LinkToProxy

  bool PropertyNode::LinkToProxy(const PropertyProxy<int>& _proxy) {
    if(m_LinkedToProxy) {
      return false;
    }
    m_Proxy.intProxy = _proxy.clone();
    m_LinkedToProxy = true;
    m_PropVal.valueType = vTypeInteger;
    return true;
  } // LinkToProxy

  bool PropertyNode::LinkToProxy(const PropertyProxy<std::string>& _proxy) {
    if(m_LinkedToProxy) {
      return false;
    }
    m_Proxy.stringProxy = _proxy.clone();
    m_LinkedToProxy = true;
    m_PropVal.valueType = vTypeString;
    return true;
  } // LinkToProxy

  bool PropertyNode::UnlinkProxy() {
    if(m_LinkedToProxy) {
      switch(GetValueType()) {
        case vTypeString:
          delete m_Proxy.stringProxy;
          m_Proxy.stringProxy = NULL;
          break;
        case vTypeInteger:
          delete m_Proxy.intProxy;
          m_Proxy.intProxy = NULL;
          break;
        case vTypeBoolean:
          delete m_Proxy.boolProxy;
          m_Proxy.boolProxy = NULL;
          break;
        default:
          assert(false);
      }
      m_LinkedToProxy = false;
      return true;
    }
    return false;
  } // UnlinkProxy


  aValueType PropertyNode::GetValueType() {
    return m_PropVal.valueType;
  } // GetValueType


  string PropertyNode::GetAsString() {
    string result;

    switch(GetValueType()) {
      case vTypeString:
        result = GetStringValue();
        break;
      case vTypeInteger:
        result = IntToString(GetIntegerValue());
        break;
      case vTypeBoolean:
        if(GetBoolValue()) {
          result = "true";
        } else {
          result = "false";
        }
        break;
      default:
        result = "";
    }
    return result;
  } // GetAsString


  void PropertyNode::AddListener(PropertyListener* _listener) const {
    m_Listeners.push_back(_listener);
  } // AddListener


  void PropertyNode::RemoveListener(PropertyListener* _listener) const {
    vector<PropertyListener*>::iterator it = std::find(m_Listeners.begin(), m_Listeners.end(), _listener);
    if(it != m_Listeners.end()) {
      m_Listeners.erase(it);
      _listener->UnregisterProperty(this);
    }
  } // RemoveListener


  PropertyNode* PropertyNode::CreateProperty(const std::string& _propPath) {
    string nextOne = GetRoot(_propPath);
    string remainder = _propPath;
    remainder.erase(0, nextOne.length() + 1);
    PropertyNode* nextNode = NULL;
    if((nextNode = GetPropertyByName(nextOne)) == NULL) {
      if(nextOne[ nextOne.length() - 1 ] == '+') {
        nextOne.erase(nextOne.length() - 1, 1);
      }
      int index = GetAndRemoveIndexFromPropertyName(nextOne);
      if(index == 0) {
        index = Count(nextOne) + 1;
      }
      nextNode = new PropertyNode(this, nextOne.c_str(), index);
    }

    if(remainder.length() > 0) {
      return nextNode->CreateProperty(remainder);
    } else {
      return nextNode;
    }
  }

  bool PropertyNode::SaveAsXML(xmlTextWriterPtr _writer) {
    int rc = xmlTextWriterStartElement(_writer, (xmlChar*)"property");
    if(rc < 0) {
      return false;
    }

    rc = xmlTextWriterWriteAttribute(_writer, (xmlChar*)"type", (xmlChar*)GetValueTypeAsString(GetValueType()));
    if(rc < 0) {
      return false;
    }

    rc = xmlTextWriterWriteAttribute(_writer, (xmlChar*)"name", (xmlChar*)GetDisplayName().c_str());
    if(rc < 0) {
      return false;
    }

    if(GetValueType() != vTypeNone) {
      xmlTextWriterWriteElement(_writer, (xmlChar*)"value", (xmlChar*)GetAsString().c_str());
    }

    for(vector<PropertyNode*>::iterator it = m_ChildNodes.begin();
         it != m_ChildNodes.end(); ++it) {
      if(!(*it)->SaveAsXML(_writer)) {
        return false;
      }
    }

    rc = xmlTextWriterEndElement(_writer);
    if(rc < 0) {
      return false;
    }
    return true;
  } // SaveAsXML


  bool PropertyNode::LoadFromNode(xmlNode* _pNode) {
    xmlAttr* nameAttr = xmlSearchAttr(_pNode, (xmlChar*)"name");
    xmlAttr* typeAttr = xmlSearchAttr(_pNode, (xmlChar*)"type");

    if(nameAttr != NULL) {
      string propName = (char*)nameAttr->children->content;
      propName = dss::GetProperty(propName);
      GetAndRemoveIndexFromPropertyName(propName);
      if(m_Name.length() > 0) {
        assert(m_Name == propName);
      }
      m_Name = propName;
      if(typeAttr != NULL) {
        aValueType valueType = GetValueTypeFromString((char*)typeAttr->children->content);
        if(valueType != vTypeNone) {
          xmlNode* valueNode = xmlSearchNode(_pNode, "value", false);
          if(valueNode != NULL) {
            switch(valueType) {
              case vTypeString:
                SetStringValue((char*)valueNode->children->content);
                break;
              case vTypeInteger:
                SetIntegerValue(atoi((char*)valueNode->children->content));
                break;
              case vTypeBoolean:
                SetBooleanValue(strcmp((char*)valueNode->children->content, "true") == 0);
                break;
              default:
                assert(false);
            }
          }
        }
      }
      xmlNode* curNode = _pNode->children;
      while(curNode != NULL) {
        if(curNode->type == XML_ELEMENT_NODE && strcmp((char*)curNode->name, "property") == 0) {
          nameAttr = xmlSearchAttr(curNode, (xmlChar*)"name");
          if(nameAttr != NULL) {
            PropertyNode* candidate = CreateProperty(string((char*)nameAttr->children->content));
            candidate->LoadFromNode(curNode);
          }
        }
        curNode = curNode->next;
      }
      return true;
    }
    return false;
  } // LoadFromNode


  void PropertyNode::NotifyListeners(void (PropertyListener::*_callback)(const PropertyNode*)) {
    vector<PropertyListener*>::iterator it;
    bool notified = false;
    for(it = m_Listeners.begin(); it != m_Listeners.end(); ++it) {
      ((*it)->*_callback)(this);
      notified = true;
    }
    if(!notified) {
      if(m_ParentNode != NULL) {
        m_ParentNode->NotifyListeners(_callback);
      }
    }
  }


  //=============================================== PropertyListener


  PropertyListener::~PropertyListener() {
    vector<const PropertyNode*>::iterator it;
    for(it = m_Properties.begin(); it != m_Properties.end(); ++it) {
      (*it)->RemoveListener(this);
    }
  } // dtor


  void PropertyListener::PropertyChanged(const PropertyNode* _changedNode) {
  } // PropertyChanged


  void PropertyListener::RegisterProperty(const PropertyNode* _node) {
    m_Properties.push_back(_node);
  } // RegisterProperty


  void PropertyListener::UnregisterProperty(const PropertyNode* _node) {
   vector<const PropertyNode*>::iterator it = std::find(m_Properties.begin(), m_Properties.end(), _node);
    if(it != m_Properties.end()) {
      m_Properties.erase(it);
    }
  } // UnregisterProperty


  //=============================================== Utilities


  string GetBasePath(const std::string& _path) {
    string result = _path;
    if(result.length() > 1) {
      string::size_type pos = result.rfind('/');
      result.erase(pos, string::npos);
    }
    if(result.length() == 0) {
      result = "/";
    }
    return result;
  } // GetBasePath

  string GetProperty(const std::string& _path) {
    string result = _path;
    string::size_type pos = result.rfind('/');
    result.erase(0, pos + 1);
    return result;
  } // GetProperty

  string GetRoot(const std::string& _path) {
    string result = _path;
    string::size_type pos = result.find('/');
    if(pos != string::npos) {
      result.erase(pos);
    }
    return result;
  } // GetRoot

  const char* GetValueTypeAsString(aValueType _value) {
    switch(_value) {
      case vTypeNone:
        return "none";
        break;
      case vTypeInteger:
        return "integer";
        break;
      case vTypeString:
        return "string";
        break;
      case vTypeBoolean:
        return "boolean";
        break;
    }
    return "unknown";
  } // GetValueTypeAsString

  aValueType GetValueTypeFromString(const char* _str) {
    string strVal = _str;
    if(strVal == "none") {
      return vTypeNone;
    } else if(strVal == "integer") {
      return vTypeInteger;
    } else if(strVal == "boolean") {
      return vTypeBoolean;
    } else if(strVal == "string") {
      return vTypeString;
    } else {
      assert(false);
    }
  } // GetValueTypeFromString


  //==================================================== Default property values

  template <> const int PropertyProxy<int>::DefaultValue = 0;
  template <> const bool PropertyProxy<bool>::DefaultValue = false;
  template <> const std::string PropertyProxy<std::string>::DefaultValue = std::string();

}
