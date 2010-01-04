/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland
    Copyright (c) 2008 Patrick Staehlin <me@packi.ch>

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

#include "propertysystem.h"

#include <iostream>
#include <sstream>
#include <algorithm>
#include <cassert>
#include <cstring>
#include <fstream>

#include <Poco/DOM/Document.h>
#include <Poco/DOM/Attr.h>
#include <Poco/DOM/Text.h>
#include <Poco/DOM/AutoPtr.h>
#include <Poco/DOM/ProcessingInstruction.h>
#include <Poco/DOM/DOMWriter.h>
#include <Poco/XML/XMLWriter.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Element.h>
#include <Poco/SAX/InputSource.h>

using Poco::XML::Document;
using Poco::XML::Attr;
using Poco::XML::Text;
using Poco::XML::ProcessingInstruction;
using Poco::XML::DOMWriter;
using Poco::XML::DOMParser;
using Poco::XML::XMLWriter;
using Poco::XML::Element;
using Poco::XML::Node;
using Poco::XML::AutoPtr;
using Poco::XML::InputSource;

#include "core/base.h"
#include "core/foreach.h"
#include "core/logger.h"

namespace dss {

  const int PROPERTY_FORMAT_VERSION = 1;

  //=============================================== PropertySystem

  PropertySystem::PropertySystem()
  : m_RootNode(new PropertyNode("/"))
  { } // ctor

  PropertySystem::~PropertySystem() {
  } // dtor

  bool PropertySystem::loadFromXML(const std::string& _fileName,
                                   PropertyNodePtr _rootNode) {
    PropertyNodePtr root = _rootNode;
    if (root == NULL) {
      root = getRootNode();
    }

    std::ifstream inFile(_fileName.c_str());

    InputSource input(inFile);
    DOMParser parser;
    AutoPtr<Document> pDoc = parser.parse(&input);
    Element* rootNode = pDoc->documentElement();
    if(rootNode->localName() != "properties") {
      Logger::getInstance()->log("PropertySystem::loadFromXML: root node must be named properties, got: '" + rootNode->localName() + "'", lsError);
      return false;
    }
    if(!rootNode->hasAttribute("version")) {
      Logger::getInstance()->log("PropertySystem::loadFromXML: missing version attribute", lsError);
      return false;
    }

    if(strToIntDef(rootNode->getAttribute("version"),-1) != PROPERTY_FORMAT_VERSION) {
      Logger::getInstance()->log(std::string("PropertySystem::loadFromXML: Version mismatch, expected ") + intToString(PROPERTY_FORMAT_VERSION) + " got " + rootNode->getAttribute("version"),
                                 lsError);
      return false;
    }

    Node* node = rootNode->firstChild();
    while(node != NULL) {
      if(node->localName() == "property") {
        return root->loadFromNode(node);
      }
      node = node->nextSibling();
    }

    return false;
  } // loadFromXML

  bool PropertySystem::saveToXML(const std::string& _fileName, PropertyNodePtr _rootNode) const {
    AutoPtr<Document> pDoc = new Document;

    AutoPtr<ProcessingInstruction> pXMLHeader = pDoc->createProcessingInstruction("xml", "version='1.0' encoding='utf-8'");
    pDoc->appendChild(pXMLHeader);

    AutoPtr<Element> pRoot = pDoc->createElement("properties");
    pRoot->setAttribute("version", intToString(PROPERTY_FORMAT_VERSION));
    pDoc->appendChild(pRoot);

    PropertyNodePtr root = _rootNode;
    if(root == NULL) {
      root = m_RootNode;
    }
    root->saveAsXML(pDoc, pRoot, 0);

    // TODO: factor those line into a function as it's a copy of
    //       model.cpp/seriespersistance.cpp/metering.cpp
    std::string tmpOut = _fileName + ".tmp";
    std::ofstream ofs(tmpOut.c_str());

    if(ofs) {
      DOMWriter writer;
      writer.setNewLine("\n");
      writer.setOptions(XMLWriter::PRETTY_PRINT);
      writer.writeNode(ofs, pDoc);

      ofs.close();

      // move it to the desired location
      rename(tmpOut.c_str(), _fileName.c_str());
    } else {
      Logger::getInstance()->log("Could not open file '" + tmpOut + "' for writing", lsFatal);
    }

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
      m_Aliased(false),
      m_Index(_index),
      m_Flags(Readable | Writeable)
  {
    memset(&m_PropVal, '\0', sizeof(aPropertyValue));
  } // ctor

  PropertyNode::~PropertyNode() {
    // tell our parent node that we're gone
    if(m_ParentNode != NULL) {
      m_ParentNode->removeChild(shared_from_this());
    }
    // un-alias our aliases
    for(std::vector<PropertyNode*>::iterator it = m_AliasedBy.begin();
        it != m_AliasedBy.end();)
    {
      (*it)->alias(PropertyNodePtr());
    }
    // remove from alias targets list
    if(m_Aliased) {
      m_Aliased = false;
      std::vector<PropertyNode*>::iterator it = find(m_AliasTarget->m_AliasedBy.begin(), m_AliasTarget->m_AliasedBy.end(), this);
      if(it != m_AliasTarget->m_AliasedBy.end()) {
        m_AliasTarget->m_AliasedBy.erase(it);
      } else {
        assert(false); // we have to be in that list, or else something went horribly wrong
      }
      m_AliasTarget = NULL;
    }
    // remove all child nodes
    for(PropertyList::iterator it = m_ChildNodes.begin();
         it != m_ChildNodes.end();)
    {
      childRemoved(*it);
      (*it)->m_ParentNode = NULL; // prevent the child-node from calling removeChild
      it = m_ChildNodes.erase(it);
    }
    // remove listeners
    for(std::vector<PropertyListener*>::iterator it = m_Listeners.begin();
        it != m_Listeners.end();)
    {
      (*it)->unregisterProperty(this);
      it = m_Listeners.erase(it);
    }
    clearValue();
  } // dtor

  PropertyNodePtr PropertyNode::removeChild(PropertyNodePtr _childNode) {
    if(m_Aliased) {
      return m_AliasTarget->removeChild(_childNode);
    } else {
      PropertyList::iterator it = std::find(m_ChildNodes.begin(), m_ChildNodes.end(), _childNode);
      if(it != m_ChildNodes.end()) {
        m_ChildNodes.erase(it);
      }
      _childNode->m_ParentNode = NULL;
      childRemoved(_childNode);
      return _childNode;
    }
  }

  void PropertyNode::addChild(PropertyNodePtr _childNode) {
    if(m_Aliased) {
      m_AliasTarget->removeChild(_childNode);
    } else {
      if(_childNode.get() == this) {
        throw std::runtime_error("Adding self as child node");
      }
      if(_childNode == NULL) {
        throw std::runtime_error("Adding NULL as child node");
      }
      if(_childNode->m_ParentNode != NULL) {
        _childNode->m_ParentNode->removeChild(_childNode);
      }
      _childNode->m_ParentNode = this;
      m_ChildNodes.push_back(_childNode);
      childAdded(_childNode);
    }
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
    if(m_Aliased) {
      return m_AliasTarget->getProperty(_propPath);
    } else {
      std::string propPath = _propPath;
      std::string propName = _propPath;
      if(endsWith(propName, "/")) {
        propName.erase(propName.size() - 1);
      }
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
    }
  } // getProperty

  int PropertyNode::getAndRemoveIndexFromPropertyName(std::string& _propName) {
    if(m_Aliased) {
      return m_AliasTarget->getAndRemoveIndexFromPropertyName(_propName);
    } else {
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
    }
  } // getAndRemoveIndexFromPropertyName

  PropertyNodePtr PropertyNode::getPropertyByName(const std::string& _name) {
    if(m_Aliased) {
      return m_AliasTarget->getPropertyByName(_name);
    } else {
      int index = 0;
      std::string propName = _name;
      index = getAndRemoveIndexFromPropertyName(propName);

      int curIndex = 0;
      int lastMatch = -1;
      int numItem = 0;
      for(PropertyList::iterator it = m_ChildNodes.begin();
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
    }
  } // getPropertyName

  int PropertyNode::count(const std::string& _propertyName) {
    if(m_Aliased) {
      return m_AliasTarget->count(_propertyName);
    } else {
      int result = 0;
      for(PropertyList::iterator it = m_ChildNodes.begin();
           it != m_ChildNodes.end(); it++) {
        PropertyNodePtr cur = *it;
        if(cur->m_Name == _propertyName) {
          result++;
        }
      }
      return result;
    }
  } // count
  
  int PropertyNode::size() const {
    return m_ChildNodes.size();
  } // size

  void PropertyNode::clearValue() {
    if(m_PropVal.valueType == vTypeString) {
      free(m_PropVal.actualValue.pString);
    }
    memset(&m_PropVal, '\0', sizeof(aPropertyValue));
  } // clearValue

  void PropertyNode::setStringValue(const char* _value) {
    if(m_Aliased) {
      m_AliasTarget->setStringValue(_value);
    } else {
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
    }
  } // setStringValue

  void PropertyNode::setStringValue(const std::string& _value) {
    setStringValue(_value.c_str());
  } // setStringValue

  void PropertyNode::setIntegerValue(const int _value) {
    if(m_Aliased) {
      m_AliasTarget->setIntegerValue(_value);
    } else {
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
    }
  } // setIntegerValue

  void PropertyNode::setBooleanValue(const bool _value) {
    if(m_Aliased) {
      m_AliasTarget->setBooleanValue(_value);
    } else {
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
    }
  } // setBooleanValue

  std::string PropertyNode::getStringValue() {
    if(m_Aliased) {
      return m_AliasTarget->getStringValue();
    } else {
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
    }
  } // getStringValue

  int PropertyNode::getIntegerValue() {
    if(m_Aliased) {
      return m_AliasTarget->getIntegerValue();
    } else {
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
    }
  } // getIntegerValue

  bool PropertyNode::getBoolValue() {
    if(m_Aliased) {
      return m_AliasTarget->getBoolValue();
    } else {
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
    }
  } // getBoolValue

  void PropertyNode::alias(PropertyNodePtr _target) {
    if(m_ChildNodes.size() > 0) {
      throw std::runtime_error("Cannot alias node if it has children");
    }
    if(m_LinkedToProxy) {
      throw std::runtime_error("Cannot alias node if it is linked to a proxy");
    }
    if(_target == NULL) {
      if(m_Aliased) {
        if(m_AliasTarget != NULL) {
          std::vector<PropertyNode*>::iterator it = find(m_AliasTarget->m_AliasedBy.begin(), m_AliasTarget->m_AliasedBy.end(), this);
          if(it != m_AliasTarget->m_AliasedBy.end()) {
            m_AliasTarget->m_AliasedBy.erase(it);
          } else {
            assert(false); // if we're not in that list something went horibly wrong
          }
          m_AliasTarget = NULL;
          m_Aliased = false;
        } else {
          throw std::runtime_error("Node is flagged as aliased but m_AliasTarget is NULL");
        }
      }
    } else {
      clearValue();
      m_AliasTarget = _target.get();
      m_Aliased = true;
      m_AliasTarget->m_AliasedBy.push_back(this);
    }
  }

  bool PropertyNode::linkToProxy(const PropertyProxy<bool>& _proxy) {
    if(m_LinkedToProxy || m_Aliased) {
      return false;
    }
    m_Proxy.boolProxy = _proxy.clone();
    m_LinkedToProxy = true;
    m_PropVal.valueType = vTypeBoolean;
    return true;
  } // linkToProxy

  bool PropertyNode::linkToProxy(const PropertyProxy<int>& _proxy) {
    if(m_LinkedToProxy || m_Aliased) {
      return false;
    }
    m_Proxy.intProxy = _proxy.clone();
    m_LinkedToProxy = true;
    m_PropVal.valueType = vTypeInteger;
    return true;
  } // linkToProxy

  bool PropertyNode::linkToProxy(const PropertyProxy<std::string>& _proxy) {
    if(m_LinkedToProxy || m_Aliased) {
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
    return m_Aliased ? m_AliasTarget->m_PropVal.valueType : m_PropVal.valueType;
  } // getValueType


  std::string PropertyNode::getAsString() {
    if(m_Aliased) {
      return m_AliasTarget->getAsString();
    } else {
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
    }
  } // getAsString

  void PropertyNode::addListener(PropertyListener* _listener) {
    _listener->registerProperty(this);
    m_Listeners.push_back(_listener);
  } // addListener

  void PropertyNode::removeListener(PropertyListener* _listener) {
    std::vector<PropertyListener*>::iterator it = std::find(m_Listeners.begin(), m_Listeners.end(), _listener);
    if(it != m_Listeners.end()) {
      m_Listeners.erase(it);
      _listener->unregisterProperty(this);
    }
  } // removeListener

  PropertyNodePtr PropertyNode::createProperty(const std::string& _propPath) {
    if(m_Aliased) {
      return m_AliasTarget->createProperty(_propPath);
    } else {
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
  }

  bool PropertyNode::saveAsXML(AutoPtr<Document>& _doc, AutoPtr<Element>& _parent, const int _flagsMask) {
    AutoPtr<Element> elem = _doc->createElement("property");
    _parent->appendChild(elem);

    elem->setAttribute("type",getValueTypeAsString(getValueType()));
    elem->setAttribute("name", getDisplayName());

    if(getValueType() != vTypeNone) {
      AutoPtr<Element> valueElem = _doc->createElement("value");
      AutoPtr<Text> textElem = _doc->createTextNode(getAsString());
      valueElem->appendChild(textElem);
      elem->appendChild(valueElem);
    }
    if(hasFlag(Archive)) {
      elem->setAttribute("archive", "true");
    }
    if(hasFlag(Readable)) {
      elem->setAttribute("readable", "true");
    }
    if(hasFlag(Writeable)) {
      elem->setAttribute("writeable", "true");
    }

    return saveChildrenAsXML(_doc, elem, _flagsMask);
  } // saveAsXML

  bool PropertyNode::saveChildrenAsXML(Poco::AutoPtr<Poco::XML::Document>& _doc, Poco::AutoPtr<Poco::XML::Element>& _parent, const int _flagsMask) {
    for(PropertyList::iterator it = m_ChildNodes.begin();
         it != m_ChildNodes.end(); ++it) {
      if((_flagsMask == Flag(0)) || (*it)->hasFlag(Flag(_flagsMask))) {
        if(!(*it)->saveAsXML(_doc, _parent, _flagsMask)) {
          return false;
        }
      }
    }
    return true;
  } // saveChildrenAsXML

  bool PropertyNode::loadFromNode(Node* _pNode) {
    Element* elem = dynamic_cast<Element*>(_pNode);

    if(elem->hasAttribute("name")) {
      std::string propName = elem->getAttribute("name");
      propName = dss::getProperty(propName);
      getAndRemoveIndexFromPropertyName(propName);
      if(m_Name.length() > 0) {
        assert(m_Name == propName);
      }
      m_Name = propName;
      if(elem->hasAttribute("type")) {
        aValueType valueType = getValueTypeFromString(elem->getAttribute("type").c_str());
        if(valueType != vTypeNone) {
          Element* valueNode = elem->getChildElement("value");
          if(valueNode != NULL) {
            switch(valueType) {
              case vTypeString:
                setStringValue(valueNode->firstChild()->getNodeValue());
                break;
              case vTypeInteger:
                setIntegerValue(strToInt(valueNode->firstChild()->getNodeValue()));
                break;
              case vTypeBoolean:
                setBooleanValue(valueNode->firstChild()->getNodeValue() == "true");
                break;
              default:
                assert(false);
            }
          }
        }
      }
      if(elem->hasAttribute("writeable")) {
        setFlag(Writeable, elem->getAttribute("writeable") == "true");
      }
      if(elem->hasAttribute("readable")) {
        setFlag(Readable, elem->getAttribute("readable") == "true");
      }
      if(elem->hasAttribute("archive")) {
        setFlag(Archive, elem->getAttribute("archive") == "true");
      }
      return loadChildrenFromNode(_pNode);
    }
    return false;
  } // loadFromNode

  bool PropertyNode::loadChildrenFromNode(Poco::XML::Node* _node) {
    Node* curNode = _node->firstChild();
    while(curNode != NULL) {
      if(curNode->localName() == "property") {
        Element* curElem = dynamic_cast<Element*>(curNode);
        if((curElem != NULL) && curElem->hasAttribute("name")) {
          PropertyNodePtr candidate = createProperty(curElem->getAttribute("name"));
          candidate->loadFromNode(curNode);
        }
      }
      curNode = curNode->nextSibling();
    }
    return true;
  } // loadChildrenFromNode

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
    bool notified = false;
    if(!m_Listeners.empty()) {
      // as the list might get modified in the listener code we need to iterate
      // over a copy of our listeners
      std::vector<PropertyListener*> copy = m_Listeners;
      std::vector<PropertyListener*>::iterator it;
      foreach(PropertyListener* pListener, copy) {
        // check if the original list still contains the listener
        if(contains(m_Listeners, pListener)) {
          (pListener->*_callback)(shared_from_this());
          notified = true;
        }
      }
    }
    if(!notified) {
      if(m_ParentNode != NULL) {
        m_ParentNode->notifyListeners(_callback);
      }
    }
    foreach(PropertyNode* prop, m_AliasedBy) {
      prop->notifyListeners(_callback);
    }
  } // notifyListeners

  void PropertyNode::notifyListeners(void (PropertyListener::*_callback)(PropertyNodePtr,PropertyNodePtr), PropertyNodePtr _node) {
    bool notified = false;
    if(!m_Listeners.empty()) {
      // as the list might get modified in the listener code we need to iterate
      // over a copy of our listeners
      std::vector<PropertyListener*> copy = m_Listeners;
      std::vector<PropertyListener*>::iterator it;
      foreach(PropertyListener* pListener, copy) {
        // check if the original list still contains the listener
        if(contains(m_Listeners, pListener)) {
          (pListener->*_callback)(shared_from_this(), _node);
          notified = true;
        }
      }
    }
    if(!notified) {
      if(m_ParentNode != NULL) {
        m_ParentNode->notifyListeners(_callback, _node);
      }
    }
    foreach(PropertyNode* prop, m_AliasedBy) {
      prop->notifyListeners(_callback, _node);
    }
  } // notifyListeners


  //=============================================== PropertyListener

  PropertyListener::~PropertyListener() {
    unsubscribe();
  } // dtor

  void PropertyListener::unsubscribe() {
    // while this does look like an infinite loop it isn't
    // as the property will call unregisterProperty in removeListener
    while(!m_Properties.empty()) {
      m_Properties.front()->removeListener(this);
    }
  } // unsubscribe

  void PropertyListener::propertyChanged(PropertyNodePtr _changedNode) {
  } // propertyChanged

  void PropertyListener::propertyRemoved(PropertyNodePtr _parent, PropertyNodePtr _child) {
  } // propertyRemoved

  void PropertyListener::propertyAdded(PropertyNodePtr _parent, PropertyNodePtr _child) {
  } // propertyAdded

  void PropertyListener::registerProperty(PropertyNode* _node) {
    m_Properties.push_back(_node);
  } // registerProperty

  void PropertyListener::unregisterProperty(PropertyNode* _node) {
    std::vector<PropertyNode*>::iterator it = std::find(m_Properties.begin(), m_Properties.end(), _node);
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
