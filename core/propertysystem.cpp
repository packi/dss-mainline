/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland
    Copyright (c) 2009 futureLAB AG, Winterthur, Switzerland
    Copyright (c) 2008 Patrick Staehlin <me@packi.ch>

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

  PropertySystem::PropertySystem()
  : m_RootNode(new PropertyNode("/"))
  { } // ctor

  PropertySystem::~PropertySystem() {
  } // dtor

  bool PropertySystem::loadFromXML(const std::string& _fileName,
                                   PropertyNodePtr _rootNode) {
    xmlDoc* doc = NULL;
    xmlNode *rootElem = NULL;

    PropertyNodePtr rootNode = _rootNode;
    if (rootNode == NULL) {
      rootNode = getRootNode();
    }

    doc = xmlParseFile(_fileName.c_str());
    if (doc == NULL) {
      std::cerr << "Error loading properties from \"" << _fileName << "\"" << std::endl;
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
                rootNode->loadFromNode(curNode);
                break;
              }
              curNode = curNode->next;
            }
          }
        }
      }
    }
    if (!versionOK) {
      std::cerr << "Version mismatch while loading properties from \"" << _fileName
          << "\"" << std::endl;
      xmlFreeDoc(doc);
    }

    return versionOK;
  } // loadFromXML


  bool PropertySystem::saveToXML(const std::string& _fileName, PropertyNodePtr _rootNode) const {
    int rc;
    xmlTextWriterPtr writer;
    PropertyNodePtr root = _rootNode;
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

    root->saveAsXML(writer);

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
  } // saveToXML


  PropertyNodePtr PropertySystem::getProperty(const std::string& _propPath) const {
    if(_propPath[ 0 ] != '/') {
      return PropertyNodePtr();
    }
    std::string propPath = _propPath;
    propPath.erase(0, 1);
    if(propPath.length() == 0) {
      return m_RootNode;
    }
    return m_RootNode->getProperty(propPath);
  } // getProperty


  PropertyNodePtr PropertySystem::createProperty(const std::string& _propPath) {
    if(_propPath[ 0 ] != '/') {
      return PropertyNodePtr();
    }
    std::string propPath = _propPath;
    propPath.erase(0, 1);
    if(propPath.length() == 0) {
      return m_RootNode;
    }
    return m_RootNode->createProperty(propPath);
  } // createProperty

  int PropertySystem::getIntValue(const std::string& _propPath) const {
    PropertyNodePtr prop = getProperty(_propPath);
    if(prop != NULL) {
      return prop->getIntegerValue();
    } else {
      return 0;
    }
  } // getIntValue

  bool PropertySystem::getBoolValue(const std::string& _propPath) const {
    PropertyNodePtr prop = getProperty(_propPath);
    if(prop != NULL) {
      return prop->getBoolValue();
    } else {
      return false;
    }
  } // getBoolValue

  std::string PropertySystem::getStringValue(const std::string& _propPath) const {
    PropertyNodePtr prop = getProperty(_propPath);
    if(prop != NULL) {
      return prop->getStringValue();
    } else {
      return std::string();
    }
  } // getStringValue

  bool PropertySystem::setIntValue(const std::string& _propPath, const int _value, bool _mayCreate, bool _mayOverwrite) {
    PropertyNodePtr prop = getProperty(_propPath);
    if((prop == NULL) &&_mayCreate) {
      PropertyNodePtr prop = createProperty(_propPath);
      prop->setIntegerValue(_value);
      return true;
    } else {
      if(prop != NULL && _mayOverwrite) {
        prop->setIntegerValue(_value);
        return true;
      } else {
        return false;
      }
    }
  } // setIntValue

  bool PropertySystem::setBoolValue(const std::string& _propPath, const bool _value, bool _mayCreate, bool _mayOverwrite) {
    PropertyNodePtr prop = getProperty(_propPath);
    if((prop == NULL) &&_mayCreate) {
      PropertyNodePtr prop = createProperty(_propPath);
      prop->setBooleanValue(_value);
      return true;
    } else {
      if(prop != NULL && _mayOverwrite) {
        prop->setBooleanValue(_value);
        return true;
      } else {
        return false;
      }
    }
  } // setBoolValue

  bool PropertySystem::setStringValue(const std::string& _propPath, const std::string& _value, bool _mayCreate, bool _mayOverwrite) {
    PropertyNodePtr prop = getProperty(_propPath);
    if((prop == NULL) &&_mayCreate) {
      PropertyNodePtr prop = createProperty(_propPath);
      prop->setStringValue(_value);
      return true;
    } else {
      if(prop != NULL && _mayOverwrite) {
        prop->setStringValue(_value);
        return true;
      } else {
        return false;
      }
    }
  } // setStringValue


  //=============================================== PropertyNode

  PropertyNode::PropertyNode(const char* _name, int _index)
    : m_ParentNode(NULL),
      m_Name(_name),
      m_LinkedToProxy(false),
      m_Index(_index)
  {
    memset(&m_PropVal, '\0', sizeof(aPropertyValue));
  } // ctor

  PropertyNode::~PropertyNode() {
    if(m_ParentNode != NULL) {
      m_ParentNode->removeChild(shared_from_this());
    }
    for(std::vector<PropertyNodePtr>::iterator it = m_ChildNodes.begin();
         it != m_ChildNodes.end();) {
      childRemoved(*it);
      (*it)->m_ParentNode = NULL; // prevent the child-node from calling removeChild
      it = m_ChildNodes.erase(it);
    }
    for(std::vector<PropertyListener*>::iterator it = m_Listeners.begin();
        it != m_Listeners.end();)
    {
      (*it)->unregisterProperty(shared_from_this());
      it = m_Listeners.erase(it);
    }
    if(m_PropVal.valueType == vTypeString) {
      free(m_PropVal.actualValue.pString);
    }
  } // dtor

  PropertyNodePtr PropertyNode::removeChild(PropertyNodePtr _childNode) {
    std::vector<PropertyNodePtr>::iterator it = std::find(m_ChildNodes.begin(), m_ChildNodes.end(), _childNode);
    if(it != m_ChildNodes.end()) {
      m_ChildNodes.erase(it);
    }
    _childNode->m_ParentNode = NULL;
    childRemoved(_childNode);
    return _childNode;
  }

  void PropertyNode::addChild(PropertyNodePtr _childNode) {
    if(_childNode.get() == this) {
      throw std::runtime_error("Adding self as child node");
    }
    if(_childNode.get() == NULL) {
      throw std::runtime_error("Adding NULL as child node");
    }
    if(_childNode->m_ParentNode != NULL) {
      _childNode->m_ParentNode->removeChild(_childNode);
    }
    _childNode->m_ParentNode = this;
    m_ChildNodes.push_back(_childNode);
    childAdded(_childNode);
  } // addChild

  const std::string& PropertyNode::getDisplayName() const {
    if(m_ParentNode->count(m_Name) > 1) {
      std::stringstream sstr;
      sstr << getName() << "[" << m_Index << "]" << std::endl;
      m_DisplayName = sstr.str();
      return m_DisplayName;
    } else {
      return m_Name;
    }
  } // getDisplayName

  PropertyNodePtr PropertyNode::getProperty(const std::string& _propPath) {
    std::string propPath = _propPath;
    std::string propName = _propPath;
    std::string::size_type slashPos = propPath.find('/');
    if(slashPos != std::string::npos) {
      propName = propPath.substr(0, slashPos);
      propPath.erase(0, slashPos + 1);
      PropertyNodePtr child = getPropertyByName(propName);
      if(child != NULL) {
        return child->getProperty(propPath);
      } else {
        return PropertyNodePtr();
      }
    } else {
      return getPropertyByName(propName);
    }
  } // getProperty

  int PropertyNode::getAndRemoveIndexFromPropertyName(std::string& _propName) {
    int result = 0;
    std::string::size_type pos = _propName.find('[');
    if(pos != std::string::npos) {
      std::string::size_type end = _propName.find(']');
      std::string indexAsString = _propName.substr(pos + 1, end - pos - 1);
      _propName.erase(pos, end);
      if(trim(indexAsString) == "last") {
        result = -1;
      } else {
        result = atoi(indexAsString.c_str());
      }
    }
    return result;
  } // getAndRemoveIndexFromPropertyName

  PropertyNodePtr PropertyNode::getPropertyByName(const std::string& _name) {
    int index = 0;
    std::string propName = _name;
    index = getAndRemoveIndexFromPropertyName(propName);

    int curIndex = 0;
    int lastMatch = -1;
    int numItem = 0;
    for(std::vector<PropertyNodePtr>::iterator it = m_ChildNodes.begin();
         it != m_ChildNodes.end(); it++) {
      numItem++;
      PropertyNodePtr cur = *it;
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
    return PropertyNodePtr();
  } // getPropertyName

  int PropertyNode::count(const std::string& _propertyName) {
    int result = 0;
    for(std::vector<PropertyNodePtr>::iterator it = m_ChildNodes.begin();
         it != m_ChildNodes.end(); it++) {
      PropertyNodePtr cur = *it;
      if(cur->m_Name == _propertyName) {
        result++;
      }
    }
    return result;
  } // count

  void PropertyNode::clearValue() {
    if(m_PropVal.valueType == vTypeString) {
      if(m_PropVal.actualValue.pString != NULL) {
        free(m_PropVal.actualValue.pString);
      }
    }
    memset(&m_PropVal, '\0', sizeof(aPropertyValue));
  } // clearValue

  void PropertyNode::setStringValue(const char* _value) {
    if(m_LinkedToProxy) {
      if(m_PropVal.valueType == vTypeString) {
        m_Proxy.stringProxy->setValue(_value);
      } else {
        std::cerr << "*** setting std::string on a non std::string property";
        throw PropertyTypeMismatch("Property-Type mismatch: " + m_Name);
      }
    } else {
      clearValue();
      if(_value != NULL) {
        m_PropVal.actualValue.pString = strdup(_value);
      }
    }
    m_PropVal.valueType = vTypeString;
    propertyChanged();
  } // setStringValue

  void PropertyNode::setStringValue(const std::string& _value) {
    setStringValue(_value.c_str());
  } // setStringValue

  void PropertyNode::setIntegerValue(const int _value) {
    if(m_LinkedToProxy) {
      if(m_PropVal.valueType == vTypeInteger) {
        m_Proxy.intProxy->setValue(_value);
      } else {
        std::cerr << "*** setting integer on a non integer property";
        throw PropertyTypeMismatch("Property-Type mismatch: " + m_Name);
      }
    } else {
      clearValue();
      m_PropVal.actualValue.integer = _value;
    }
    m_PropVal.valueType = vTypeInteger;
    propertyChanged();
  } // setIntegerValue

  void PropertyNode::setBooleanValue(const bool _value) {
    if(m_LinkedToProxy) {
      if(m_PropVal.valueType == vTypeBoolean) {
        m_Proxy.boolProxy->setValue(_value);
      } else {
        std::cerr << "*** setting bool on a non booleanproperty";
        throw PropertyTypeMismatch("Property-Type mismatch: " + m_Name);
      }
    } else {
      clearValue();
      m_PropVal.actualValue.boolean = _value;
    }
    m_PropVal.valueType = vTypeBoolean;
    propertyChanged();
  } // setBooleanValue

  std::string PropertyNode::getStringValue() {
    if(m_PropVal.valueType == vTypeString) {
      if(m_LinkedToProxy) {
        return m_Proxy.stringProxy->getValue();
      } else {
        return m_PropVal.actualValue.pString;
      }
    } else {
      std::cerr << "Property-Type mismatch: " << m_Name << std::endl;
      throw PropertyTypeMismatch("Property-Type mismatch: " + m_Name);
    }
  } // getStringValue

  int PropertyNode::getIntegerValue() {
    if(m_PropVal.valueType == vTypeInteger) {
      if(m_LinkedToProxy) {
        return m_Proxy.intProxy->getValue();
      } else {
        return m_PropVal.actualValue.integer;
      }
    } else {
      std::cerr << "Property-Type mismatch: " << m_Name << std::endl;
      throw PropertyTypeMismatch("Property-Type mismatch: " + m_Name);
    }
  } // getIntegerValue

  bool PropertyNode::getBoolValue() {
    if(m_PropVal.valueType == vTypeBoolean) {
      if(m_LinkedToProxy) {
        return m_Proxy.boolProxy->getValue();
      } else {
        return m_PropVal.actualValue.boolean;
      }
    } else {
      std::cerr << "Property-Type mismatch: " << m_Name << std::endl;
      throw PropertyTypeMismatch("Property-Type mismatch: " + m_Name);
    }
  } // getBoolValue

  bool PropertyNode::linkToProxy(const PropertyProxy<bool>& _proxy) {
    if(m_LinkedToProxy) {
      return false;
    }
    m_Proxy.boolProxy = _proxy.clone();
    m_LinkedToProxy = true;
    m_PropVal.valueType = vTypeBoolean;
    return true;
  } // linkToProxy

  bool PropertyNode::linkToProxy(const PropertyProxy<int>& _proxy) {
    if(m_LinkedToProxy) {
      return false;
    }
    m_Proxy.intProxy = _proxy.clone();
    m_LinkedToProxy = true;
    m_PropVal.valueType = vTypeInteger;
    return true;
  } // linkToProxy

  bool PropertyNode::linkToProxy(const PropertyProxy<std::string>& _proxy) {
    if(m_LinkedToProxy) {
      return false;
    }
    m_Proxy.stringProxy = _proxy.clone();
    m_LinkedToProxy = true;
    m_PropVal.valueType = vTypeString;
    return true;
  } // linkToProxy

  bool PropertyNode::unlinkProxy() {
    if(m_LinkedToProxy) {
      switch(getValueType()) {
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
  } // unlinkProxy


  aValueType PropertyNode::getValueType() {
    return m_PropVal.valueType;
  } // getValueType


  std::string PropertyNode::getAsString() {
    std::string result;

    switch(getValueType()) {
      case vTypeString:
        result = getStringValue();
        break;
      case vTypeInteger:
        result = intToString(getIntegerValue());
        break;
      case vTypeBoolean:
        if(getBoolValue()) {
          result = "true";
        } else {
          result = "false";
        }
        break;
      default:
        result = "";
    }
    return result;
  } // getAsString

  void PropertyNode::addListener(PropertyListener* _listener) {
    m_Listeners.push_back(_listener);
  } // addListener

  void PropertyNode::removeListener(PropertyListener* _listener) {
    std::vector<PropertyListener*>::iterator it = std::find(m_Listeners.begin(), m_Listeners.end(), _listener);
    if(it != m_Listeners.end()) {
      m_Listeners.erase(it);
      _listener->unregisterProperty(shared_from_this());
    }
  } // removeListener

  PropertyNodePtr PropertyNode::createProperty(const std::string& _propPath) {
    std::string nextOne = getRoot(_propPath);
    std::string remainder = _propPath;
    remainder.erase(0, nextOne.length() + 1);
    PropertyNodePtr nextNode;
    if((nextNode = getPropertyByName(nextOne)) == NULL) {
      if(nextOne[ nextOne.length() - 1 ] == '+') {
        nextOne.erase(nextOne.length() - 1, 1);
      }
      int index = getAndRemoveIndexFromPropertyName(nextOne);
      if(index == 0) {
        index = count(nextOne) + 1;
      }
      nextNode.reset(new PropertyNode(nextOne.c_str(), index));
      addChild(nextNode);
    }

    if(remainder.length() > 0) {
      return nextNode->createProperty(remainder);
    } else {
      return nextNode;
    }
  }

  bool PropertyNode::saveAsXML(xmlTextWriterPtr _writer) {
    int rc = xmlTextWriterStartElement(_writer, (xmlChar*)"property");
    if(rc < 0) {
      return false;
    }

    rc = xmlTextWriterWriteAttribute(_writer, (xmlChar*)"type", (xmlChar*)getValueTypeAsString(getValueType()));
    if(rc < 0) {
      return false;
    }

    rc = xmlTextWriterWriteAttribute(_writer, (xmlChar*)"name", (xmlChar*)getDisplayName().c_str());
    if(rc < 0) {
      return false;
    }

    if(getValueType() != vTypeNone) {
      xmlTextWriterWriteElement(_writer, (xmlChar*)"value", (xmlChar*)getAsString().c_str());
    }

    for(std::vector<PropertyNodePtr>::iterator it = m_ChildNodes.begin();
         it != m_ChildNodes.end(); ++it) {
      if(!(*it)->saveAsXML(_writer)) {
        return false;
      }
    }

    rc = xmlTextWriterEndElement(_writer);
    if(rc < 0) {
      return false;
    }
    return true;
  } // saveAsXML

  bool PropertyNode::loadFromNode(xmlNode* _pNode) {
    xmlAttr* nameAttr = xmlSearchAttr(_pNode, (xmlChar*)"name");
    xmlAttr* typeAttr = xmlSearchAttr(_pNode, (xmlChar*)"type");

    if(nameAttr != NULL) {
      std::string propName = (char*)nameAttr->children->content;
      propName = dss::getProperty(propName);
      getAndRemoveIndexFromPropertyName(propName);
      if(m_Name.length() > 0) {
        assert(m_Name == propName);
      }
      m_Name = propName;
      if(typeAttr != NULL) {
        aValueType valueType = getValueTypeFromString((char*)typeAttr->children->content);
        if(valueType != vTypeNone) {
          xmlNode* valueNode = xmlSearchNode(_pNode, "value", false);
          if(valueNode != NULL) {
            switch(valueType) {
              case vTypeString:
                setStringValue((char*)valueNode->children->content);
                break;
              case vTypeInteger:
                setIntegerValue(atoi((char*)valueNode->children->content));
                break;
              case vTypeBoolean:
                setBooleanValue(strcmp((char*)valueNode->children->content, "true") == 0);
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
            PropertyNodePtr candidate = createProperty(std::string((char*)nameAttr->children->content));
            candidate->loadFromNode(curNode);
          }
        }
        curNode = curNode->next;
      }
      return true;
    }
    return false;
  } // loadFromNode

  void PropertyNode::propertyChanged() {
    notifyListeners(&PropertyListener::propertyChanged);
  } // propertyChanged

  void PropertyNode::childAdded(PropertyNodePtr _child) {
    notifyListeners(&PropertyListener::propertyAdded, _child);
  } // childAdded

  void PropertyNode::childRemoved(PropertyNodePtr _child) {
    notifyListeners(&PropertyListener::propertyRemoved, _child);
  } // childRemoved

  void PropertyNode::notifyListeners(void (PropertyListener::*_callback)(PropertyNodePtr)) {
    std::vector<PropertyListener*>::iterator it;
    bool notified = false;
    for(it = m_Listeners.begin(); it != m_Listeners.end(); ++it) {
      ((*it)->*_callback)(shared_from_this());
      notified = true;
    }
    if(!notified) {
      if(m_ParentNode != NULL) {
        m_ParentNode->notifyListeners(_callback);
      }
    }
  } // notifyListeners

  void PropertyNode::notifyListeners(void (PropertyListener::*_callback)(PropertyNodePtr,PropertyNodePtr), PropertyNodePtr _node) {
    std::vector<PropertyListener*>::iterator it;
    bool notified = false;
    for(it = m_Listeners.begin(); it != m_Listeners.end(); ++it) {
      ((*it)->*_callback)(shared_from_this(), _node);
      notified = true;
    }
    if(!notified) {
      if(m_ParentNode != NULL) {
        m_ParentNode->notifyListeners(_callback, _node);
      }
    }
  } // notifyListeners


  //=============================================== PropertyListener

  PropertyListener::~PropertyListener() {
    std::vector<PropertyNodePtr>::iterator it;
    for(it = m_Properties.begin(); it != m_Properties.end(); ++it) {
      (*it)->removeListener(this);
    }
  } // dtor

  void PropertyListener::propertyChanged(PropertyNodePtr _changedNode) {
  } // propertyChanged

  void PropertyListener::propertyRemoved(PropertyNodePtr _parent, PropertyNodePtr _child) {
  } // propertyRemoved

  void PropertyListener::propertyAdded(PropertyNodePtr _parent, PropertyNodePtr _child) {
  } // propertyAdded

  void PropertyListener::registerProperty(PropertyNodePtr _node) {
    m_Properties.push_back(_node);
  } // registerProperty

  void PropertyListener::unregisterProperty(PropertyNodePtr _node) {
   std::vector<PropertyNodePtr>::iterator it = std::find(m_Properties.begin(), m_Properties.end(), _node);
    if(it != m_Properties.end()) {
      m_Properties.erase(it);
    }
  } // unregisterProperty


  //=============================================== Utilities

  std::string getBasePath(const std::string& _path) {
    std::string result = _path;
    if(result.length() > 1) {
      std::string::size_type pos = result.rfind('/');
      result.erase(pos, std::string::npos);
    }
    if(result.length() == 0) {
      result = "/";
    }
    return result;
  } // getBasePath

  std::string getProperty(const std::string& _path) {
    std::string result = _path;
    std::string::size_type pos = result.rfind('/');
    result.erase(0, pos + 1);
    return result;
  } // getProperty

  std::string getRoot(const std::string& _path) {
    std::string result = _path;
    std::string::size_type pos = result.find('/');
    if(pos != std::string::npos) {
      result.erase(pos);
    }
    return result;
  } // getRoot

  const char* getValueTypeAsString(aValueType _value) {
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
  } // getValueTypeAsString

  aValueType getValueTypeFromString(const char* _str) {
    std::string strVal = _str;
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
  } // getValueTypeFromString


  //==================================================== Default property values

  template <> const int PropertyProxy<int>::DefaultValue = 0;
  template <> const bool PropertyProxy<bool>::DefaultValue = false;
  template <> const std::string PropertyProxy<std::string>::DefaultValue = std::string();

}
