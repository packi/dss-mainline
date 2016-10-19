/*
    Copyright (c) 2009,2010,2012 digitalSTROM.org, Zurich, Switzerland
    Copyright (c) 2008 Patrick Staehlin <me@packi.ch>

    Authors: Patrick Staehlin, futureLAB AG <pstaehlin@futurelab.ch>
            Christian Hitz, aizo AG <christian.hitz@aizo.com>
            Sergey 'Jin' Bostandzhyan <jin@dev.digitalstrom.org>


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

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif


#include "propertysystem.h"

#include <iostream>
#include <sstream>
#include <algorithm>
#include <cassert>
#include <cstring>
#include <fstream>
#include <boost/make_shared.hpp>

#include "src/base.h"
#include "src/foreach.h"
#include "src/logger.h"

#include "src/security/privilege.h"
#include "src/security/security.h"
#include "src/security/user.h"
#include "src/expatparser.h"
#include "src/util.h"

namespace dss {

  const int PROPERTY_FORMAT_VERSION = 1;

  //=============================================== Util

  bool loadFromXML(const std::string& _fileName, PropertyNodePtr _rootNode) {
    assert(_rootNode != NULL);
    boost::shared_ptr<PropertyParser> pp = boost::make_shared<PropertyParser>();
    return pp->loadFromXML(_fileName, _rootNode);
  } // loadFromXML

  bool saveToXML(const std::string& _fileName, PropertyNodePtr root, const int _flagsMask) {
    std::string tmpOut = _fileName + ".tmp";
    std::ofstream ofs(tmpOut.c_str());
    assert(root != NULL);

    if (ofs) {
      boost::recursive_mutex::scoped_lock lock(PropertyNode::m_GlobalMutex);
      int indent = 0;

      ofs << "<?xml version=\"1.0\" encoding=\"utf-8\"?>" << std::endl;
      ofs << "<properties version=\"1\">" << std::endl;
      root->saveAsXML(ofs, indent + 1, _flagsMask);
      ofs << "</properties>" << std::endl;

      ofs.close();

      syncFile(tmpOut);

      saveValidatedXML(tmpOut, _fileName);
    }

    return true;
  } // saveToXML

  //=============================================== PropertySystem

  __DEFINE_LOG_CHANNEL__(PropertySystem, lsInfo);

  PropertySystem::PropertySystem()
  : m_RootNode(new PropertyNode("/"))
  { } // ctor

  PropertySystem::~PropertySystem() {
  } // dtor

  PropertyNodePtr PropertySystem::getProperty(const std::string& _propPath) const {
    if (_propPath[ 0 ] != '/') {
      return PropertyNodePtr();
    }
    std::string propPath = _propPath;
    propPath.erase(0, 1);
    if (propPath.length() == 0) {
      return m_RootNode;
    }
    return m_RootNode->getProperty(propPath);
  } // getProperty

  PropertyNodePtr PropertySystem::createProperty(const std::string& _propPath) {
    if (_propPath[ 0 ] != '/') {
      return PropertyNodePtr();
    }
    std::string propPath = _propPath;
    propPath.erase(0, 1);
    if (propPath.length() == 0) {
      return m_RootNode;
    }
    return m_RootNode->createProperty(propPath);
  } // createProperty

  int PropertySystem::getIntValue(const std::string& _propPath) const {
    PropertyNodePtr prop = getProperty(_propPath);
    if (prop != NULL) {
      return prop->getIntegerValue();
    } else {
      return 0;
    }
  } // getIntValue

  bool PropertySystem::getBoolValue(const std::string& _propPath) const {
    PropertyNodePtr prop = getProperty(_propPath);
    if (prop != NULL) {
      return prop->getBoolValue();
    } else {
      return false;
    }
  } // getBoolValue

  std::string PropertySystem::getStringValue(const std::string& _propPath) const {
    PropertyNodePtr prop = getProperty(_propPath);
    if (prop != NULL) {
      return prop->getStringValue();
    } else {
      return std::string();
    }
  } // getStringValue

  bool PropertySystem::setIntValue(const std::string& _propPath, const int _value, bool _mayCreate, bool _mayOverwrite) {
    PropertyNodePtr prop = getProperty(_propPath);
    if ((prop == NULL) &&_mayCreate) {
      PropertyNodePtr prop = createProperty(_propPath);
      prop->setIntegerValue(_value);
      return true;
    } else {
      if (prop != NULL && _mayOverwrite) {
        prop->setIntegerValue(_value);
        return true;
      } else {
        return false;
      }
    }
  } // setIntValue

  bool PropertySystem::setBoolValue(const std::string& _propPath, const bool _value, bool _mayCreate, bool _mayOverwrite) {
    PropertyNodePtr prop = getProperty(_propPath);
    if ((prop == NULL) &&_mayCreate) {
      PropertyNodePtr prop = createProperty(_propPath);
      prop->setBooleanValue(_value);
      return true;
    } else {
      if (prop != NULL && _mayOverwrite) {
        prop->setBooleanValue(_value);
        return true;
      } else {
        return false;
      }
    }
  } // setBoolValue

  bool PropertySystem::setStringValue(const std::string& _propPath, const std::string& _value, bool _mayCreate, bool _mayOverwrite) {
    PropertyNodePtr prop = getProperty(_propPath);
    if ((prop == NULL) &&_mayCreate) {
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

  __DEFINE_LOG_CHANNEL__(PropertyNode, lsInfo);

  boost::atomic<int> PropertyNode::sm_NodeCounter;
  std::vector<PropertyNodePtr> PropertyNode::sm_EmptyChildNodes;

  PropertyNode::PropertyNode(const char* _name, int _index)
    : m_Name(_name),
      m_ChildNodes(NULL),
      m_AliasedBy(NULL),
      m_Listeners(NULL),
      m_ParentNode(NULL),
      m_AliasTarget(NULL),
      m_Privileges(NULL),
      m_Index(_index),
      m_Flags(Readable | Writeable)
  {
    m_Proxy.iValue = 0;
    sm_NodeCounter++;
    memset(&m_PropVal, '\0', sizeof(aPropertyValue));
  } // ctor

  PropertyNode::~PropertyNode() {
    // remove listeners
    boost::recursive_mutex::scoped_lock lock(m_GlobalMutex);
    if (m_Listeners) {
      for (std::vector<PropertyListener*>::iterator it = m_Listeners->begin(); it != m_Listeners->end();)
      {
        (*it)->unregisterProperty(this);
        it = m_Listeners->erase(it);
      }
    }

    // tell our parent node that we're gone
    if(m_ParentNode != NULL) {
      m_ParentNode->removeChild(shared_from_this());
    }

    // un-alias our aliases
    if (m_AliasedBy) {
      for (std::vector<PropertyNode*>::iterator it = m_AliasedBy->begin(); it != m_AliasedBy->end(); ) {
        size_t prevSize = m_AliasedBy->size();
        (*it)->alias(PropertyNodePtr());
        if (m_AliasedBy->size() >= prevSize) {
          it = m_AliasedBy->erase(it);
        }
      }
      delete m_AliasedBy;
      m_AliasedBy = NULL;
    }

    if (IsLinkedToProxy()) {
      switch(m_PropVal.valueType) {
      case vTypeInteger:
        delete m_Proxy.intProxy;
        break;
      case vTypeUnsignedInteger:
        delete m_Proxy.uintProxy;
        break;
      case vTypeString:
        delete m_Proxy.stringProxy;
        break;
      case vTypeBoolean:
        delete m_Proxy.boolProxy;
        break;
      case vTypeFloating:
        delete m_Proxy.floatingProxy;
        break;
      case vTypeNone:
        break;
      }
      m_Proxy.iValue = 0;
    }

    // remove from alias targets list
    if (m_AliasTarget && m_AliasTarget->m_AliasedBy) {
      std::vector<PropertyNode*>::iterator it =
          find(m_AliasTarget->m_AliasedBy->begin(), m_AliasTarget->m_AliasedBy->end(), this);
      if (it != m_AliasTarget->m_AliasedBy->end()) {
        m_AliasTarget->m_AliasedBy->erase(it);
      } else {
        assert(false); // we have to be in that list, or else something went horribly wrong
      }
      m_AliasTarget = NULL;
    }

    // remove all child nodes
    if (m_ChildNodes) {
      for (std::vector<PropertyNodePtr>::iterator it = m_ChildNodes->begin(); it != m_ChildNodes->end();) {
        childRemoved(*it);
        (*it)->m_ParentNode = NULL; // prevent the child-node from calling removeChild
        it = m_ChildNodes->erase(it);
      }
      delete m_ChildNodes;
      m_ChildNodes = NULL;
    }

    clearValue();
    sm_NodeCounter--;
  } // dtor

  const std::vector<PropertyNodePtr>& PropertyNode::getChildNodes() const {
    const std::vector<PropertyNodePtr>* pOut = m_AliasTarget ? m_AliasTarget->m_ChildNodes : m_ChildNodes;
    return pOut ? *pOut : sm_EmptyChildNodes;
  }

  PropertyNodePtr PropertyNode::removeChild(PropertyNodePtr _childNode) {
    if (m_AliasTarget) {
      return m_AliasTarget->removeChild(_childNode);
    } else if (NULL == m_ChildNodes) {
      return PropertyNodePtr();
    } else {
      boost::recursive_mutex::scoped_lock lock(m_GlobalMutex);
      PropertyList::iterator it = std::find(m_ChildNodes->begin(), m_ChildNodes->end(), _childNode);
      if (it != m_ChildNodes->end()) {
        m_ChildNodes->erase(it);
      }
      lock.unlock();
      _childNode->m_ParentNode = NULL;
      childRemoved(_childNode);
      return _childNode;
    }
  }

  void PropertyNode::addChild(PropertyNodePtr _childNode) {
    checkWriteAccess();
    if (m_AliasTarget) {
      m_AliasTarget->addChild(_childNode);
    } else {
      if (_childNode.get() == this) {
        throw std::runtime_error("Adding self as child node");
      }
      if (_childNode == NULL) {
        throw std::runtime_error("Adding NULL as child node");
      }
      if (_childNode->m_ParentNode != NULL) {
        _childNode->m_ParentNode->removeChild(_childNode);
      }
      boost::recursive_mutex::scoped_lock lock(m_GlobalMutex);
      if (NULL == m_ChildNodes) {
        m_ChildNodes = new (std::vector<PropertyNodePtr>);
      }
      _childNode->m_ParentNode = this;
      int maxIndex = -1;
      for (unsigned int iChild = 0; iChild < m_ChildNodes->size(); iChild++) {
        if (m_ChildNodes->at(iChild)->getName() == _childNode->getName()) {
          maxIndex = std::max(m_ChildNodes->at(iChild)->m_Index, maxIndex);
        }
      }
      _childNode->m_Index = maxIndex + 1;
      m_ChildNodes->push_back(_childNode);
      lock.unlock();
      childAdded(_childNode);
    }
  } // addChild

  const std::string& PropertyNode::getDisplayName() const {
    if (m_ParentNode && (m_ParentNode->count(m_Name) > 1)) {
      std::stringstream sstr;
      sstr << getName() << "[" << m_Index << "]";
      m_DisplayName = sstr.str();
      return m_DisplayName;
    } else {
      return m_Name;
    }
  } // getDisplayName

  PropertyNodePtr PropertyNode::getProperty(const std::string& _propPath) {
    if (m_AliasTarget) {
      return m_AliasTarget->getProperty(_propPath);
    } else {
      if (_propPath.empty() || _propPath == "/") {
        // it's us, stupid
        return shared_from_this();
      }

      std::string head, tail = _propPath;
      head = carCdrPath(tail);
      assert(!head.empty());

      PropertyNodePtr child = getPropertyByName(head);
      if (child == NULL) {
        return PropertyNodePtr();
      }
      return tail.empty() ? child : child->getProperty(tail);
    }
  } // getProperty

  int PropertyNode::getAndRemoveIndexFromPropertyName(std::string& _propName) {
    if (m_AliasTarget) {
      return m_AliasTarget->getAndRemoveIndexFromPropertyName(_propName);
    } else {
      int result = 0;
      std::string::size_type pos = _propName.find('[');
      if(pos != std::string::npos) {
        std::string::size_type end = _propName.find(']');
        std::string indexAsString = _propName.substr(pos + 1, end - pos - 1);
        _propName.erase(pos, end);
        if (trim(indexAsString) == "last") {
          result = -1;
        } else {
          result = atoi(indexAsString.c_str());
        }
      }
      return result;
    }
  } // getAndRemoveIndexFromPropertyName

  PropertyNodePtr PropertyNode::getPropertyByName(const std::string& _name) {
    if (m_AliasTarget) {
      return m_AliasTarget->getPropertyByName(_name);
    } else if (NULL == m_ChildNodes ) {
      return PropertyNodePtr();
    } else {
      int index = 0;
      std::string propName = _name;
      index = getAndRemoveIndexFromPropertyName(propName);

      int lastMatch = -1;
      int numItem = 0;
      boost::recursive_mutex::scoped_lock lock(m_GlobalMutex);
      PropertyList::iterator it = m_ChildNodes->begin();
      PropertyList::iterator end = m_ChildNodes->end();
      for (; it != end; it++) {
        numItem++;
        if ((*it)->m_Name == propName) {
          if ((*it)->m_Index == index) {
            return (*it);
          }
          lastMatch = numItem - 1;
        }
      }
      if (index == -1) {
        if (lastMatch != -1) {
          return m_ChildNodes->at(lastMatch);
        }
      }
      return PropertyNodePtr();
    }
  } // getPropertyName

  int PropertyNode::count(const std::string& _propertyName) {
    if (m_AliasTarget) {
      return m_AliasTarget->count(_propertyName);
    } else if (NULL == m_ChildNodes ) {
      return 0;
    } else {
      int result = 0;
      boost::recursive_mutex::scoped_lock lock(m_GlobalMutex);
      for (PropertyList::iterator it = m_ChildNodes->begin();
           it != m_ChildNodes->end(); it++) {
        PropertyNodePtr cur = *it;
        if (cur->m_Name == _propertyName) {
          result++;
        }
      }
      return result;
    }
  } // count

  int PropertyNode::size() {
    return m_ChildNodes ? m_ChildNodes->size() : 0;
  } // size

  void PropertyNode::clearValue() {
    if (m_PropVal.valueType == vTypeString) {
      free(m_PropVal.actualValue.pString);
    }
    memset(&m_PropVal, '\0', sizeof(aPropertyValue));
  } // clearValue

  void PropertyNode::setStringValue(const char* _value) {
    checkWriteAccess();
    if (m_AliasTarget) {
      m_AliasTarget->setStringValue(_value);
    } else {
      if (IsLinkedToProxy()) {
        if (m_PropVal.valueType == vTypeString) {
          m_Proxy.stringProxy->setValue(_value);
        } else {
          Logger::getInstance()->log("*** setting std::string on a non std::string property", lsError);
          throw PropertyTypeMismatch("Property-Type mismatch: " + m_Name.get());
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
    checkWriteAccess();
    if (m_AliasTarget) {
      m_AliasTarget->setIntegerValue(_value);
    } else {
      if (IsLinkedToProxy()) {
        if(m_PropVal.valueType == vTypeInteger) {
          m_Proxy.intProxy->setValue(_value);
        } else {
          Logger::getInstance()->log("*** setting integer on a non integer property", lsError);
          throw PropertyTypeMismatch("Property-Type mismatch: " + m_Name.get());
        }
      } else {
        clearValue();
        m_PropVal.actualValue.integer = _value;
      }
      m_PropVal.valueType = vTypeInteger;
      propertyChanged();
    }
  } // setIntegerValue

  void PropertyNode::setUnsignedIntegerValue(const uint32_t _value) {
    checkWriteAccess();
    if (m_AliasTarget) {
      m_AliasTarget->setUnsignedIntegerValue(_value);
    } else {
      if (IsLinkedToProxy()) {
        if(m_PropVal.valueType == vTypeUnsignedInteger) {
          m_Proxy.uintProxy->setValue(_value);
        } else {
          Logger::getInstance()->log("*** setting unsigned integer on a non unsigned integer property", lsError);
          throw PropertyTypeMismatch("Property-Type mismatch: " + m_Name.get());
        }
      } else {
        clearValue();
        m_PropVal.actualValue.unsignedinteger = _value;
      }
      m_PropVal.valueType = vTypeUnsignedInteger;
      propertyChanged();
    }
  } // setUnsignedIntegerValue

  void PropertyNode::setBooleanValue(const bool _value) {
    checkWriteAccess();
    if (m_AliasTarget) {
      m_AliasTarget->setBooleanValue(_value);
    } else {
      if (IsLinkedToProxy()) {
        if(m_PropVal.valueType == vTypeBoolean) {
          m_Proxy.boolProxy->setValue(_value);
        } else {
          Logger::getInstance()->log("*** setting bool on a non booleanproperty");
          throw PropertyTypeMismatch("Property-Type mismatch: " + m_Name.get());
        }
      } else {
        clearValue();
        m_PropVal.actualValue.boolean = _value;
      }
      m_PropVal.valueType = vTypeBoolean;
      propertyChanged();
    }
  } // setBooleanValue

  void PropertyNode::setFloatingValue(const double _value) {
    checkWriteAccess();
    if (m_AliasTarget) {
      m_AliasTarget->setFloatingValue(_value);
    } else {
      if (IsLinkedToProxy()) {
        if (m_PropVal.valueType == vTypeFloating) {
          m_Proxy.floatingProxy->setValue(_value);
        } else {
          Logger::getInstance()->log("*** setting float on a non floating-property");
          throw PropertyTypeMismatch("Property-Type mismatch: " + m_Name.get());
        }
      } else {
        clearValue();
        m_PropVal.actualValue.floating = _value;
      }
      m_PropVal.valueType = vTypeFloating;
      propertyChanged();
    }
  } // setBooleanValue

  std::string PropertyNode::getStringValue() const {
    if (m_AliasTarget) {
      return m_AliasTarget->getStringValue();
    } else {
      if (m_PropVal.valueType == vTypeString) {
        if (IsLinkedToProxy()) {
          return m_Proxy.stringProxy->getValue();
        } else {
          return m_PropVal.actualValue.pString;
        }
      } else {
//        std::cerr << "Property-Type mismatch: " << m_Name << std::endl;
        throw PropertyTypeMismatch("Property-Type mismatch: " + m_Name.get());
      }
    }
  } // getStringValue

  int PropertyNode::getIntegerValue() const {
    if (m_AliasTarget) {
      return m_AliasTarget->getIntegerValue();
    } else {
      if (m_PropVal.valueType == vTypeInteger) {
        if (IsLinkedToProxy()) {
          return m_Proxy.intProxy->getValue();
        } else {
          return m_PropVal.actualValue.integer;
        }
      } else {
 //       std::cerr << "Property-Type mismatch: " << m_Name << std::endl;
        throw PropertyTypeMismatch("Property-Type mismatch: " + m_Name.get());
      }
    }
  } // getIntegerValue

  uint32_t PropertyNode::getUnsignedIntegerValue() const {
    if (m_AliasTarget) {
      return m_AliasTarget->getUnsignedIntegerValue();
    } else {
      if (m_PropVal.valueType == vTypeUnsignedInteger) {
        if (IsLinkedToProxy()) {
          return m_Proxy.uintProxy->getValue();
        } else {
          return m_PropVal.actualValue.unsignedinteger;
        }
      } else {
 //       std::cerr << "Property-Type mismatch: " << m_Name << std::endl;
        throw PropertyTypeMismatch("Property-Type mismatch: " + m_Name.get());
      }
    }
  } // getUnsignedIntegerValue

  bool PropertyNode::getBoolValue() const {
    if (m_AliasTarget) {
      return m_AliasTarget->getBoolValue();
    } else {
      if (m_PropVal.valueType == vTypeBoolean) {
        if (IsLinkedToProxy()) {
          return m_Proxy.boolProxy->getValue();
        } else {
          return m_PropVal.actualValue.boolean;
        }
      } else {
//        std::cerr << "Property-Type mismatch: " << m_Name << std::endl;
        throw PropertyTypeMismatch("Property-Type mismatch: " + m_Name.get());
      }
    }
  } // getBoolValue

  double PropertyNode::getFloatingValue() const {
    if (m_AliasTarget) {
      return m_AliasTarget->getFloatingValue();
    } else {
      if (m_PropVal.valueType == vTypeFloating) {
        if (IsLinkedToProxy()) {
          return m_Proxy.floatingProxy->getValue();
        } else {
          return m_PropVal.actualValue.floating;
        }
      } else {
//        std::cerr << "Property-Type mismatch: " << m_Name << std::endl;
        throw PropertyTypeMismatch("Property-Type mismatch: " + m_Name.get());
      }
    }
  } // getBoolValue

  void PropertyNode::alias(PropertyNodePtr _target) {
    checkWriteAccess();
    if (m_ChildNodes && m_ChildNodes->size() > 0) {
      throw std::runtime_error("Cannot alias node if it has children");
    }
    if (IsLinkedToProxy()) {
      throw std::runtime_error("Cannot alias node if it is linked to a proxy");
    }
    boost::recursive_mutex::scoped_lock lock(m_GlobalMutex);
    if (_target == NULL) {
      if (m_AliasTarget) {
        std::vector<PropertyNode*>::iterator it =
            find(m_AliasTarget->m_AliasedBy->begin(), m_AliasTarget->m_AliasedBy->end(), this);
        if (it != m_AliasTarget->m_AliasedBy->end()) {
          m_AliasTarget->m_AliasedBy->erase(it);
        } else {
          assert(false); // if we're not in that list something went horibly wrong
        }
        m_AliasTarget = NULL;
      }
    } else {
      clearValue();
      m_AliasTarget = _target.get();
      if (m_AliasTarget->m_AliasedBy == NULL) {
        m_AliasTarget->m_AliasedBy = new (std::vector<PropertyNode*>);
      }
      m_AliasTarget->m_AliasedBy->push_back(this);
    }
  }

  bool PropertyNode::linkToProxy(const PropertyProxy<bool>& _proxy) {
    if (IsLinkedToProxy() || m_AliasTarget) {
      return false;
    }
    m_Proxy.boolProxy = _proxy.clone();
    m_PropVal.valueType = vTypeBoolean;
    return true;
  } // linkToProxy

  bool PropertyNode::linkToProxy(const PropertyProxy<int>& _proxy) {
    if (IsLinkedToProxy() || m_AliasTarget) {
      return false;
    }
    m_Proxy.intProxy = _proxy.clone();
    m_PropVal.valueType = vTypeInteger;
    return true;
  } // linkToProxy

  bool PropertyNode::linkToProxy(const PropertyProxy<std::string>& _proxy) {
    if (IsLinkedToProxy() || m_AliasTarget) {
      return false;
    }
    m_Proxy.stringProxy = _proxy.clone();
    m_PropVal.valueType = vTypeString;
    return true;
  } // linkToProxy

  bool PropertyNode::linkToProxy(const PropertyProxy<uint32_t>& _proxy) {
    if (IsLinkedToProxy() || m_AliasTarget) {
      return false;
    }
    m_Proxy.uintProxy = _proxy.clone();
    m_PropVal.valueType = vTypeUnsignedInteger;
    return true;
  } // linkToProxy


  bool PropertyNode::linkToProxy(const PropertyProxy<double>& _proxy) {
    if (IsLinkedToProxy() || m_AliasTarget) {
      return false;
    }
    m_Proxy.floatingProxy = _proxy.clone();
    m_PropVal.valueType = vTypeFloating;
    return true;
  } // linkToProxy

  bool PropertyNode::unlinkProxy(bool _recurse) {
    boost::recursive_mutex::scoped_lock lock(m_GlobalMutex);
    if (_recurse && m_ChildNodes) {
      for (std::vector<PropertyNodePtr>::iterator it = m_ChildNodes->begin(); it != m_ChildNodes->end(); it++) {
        (*it)->unlinkProxy(_recurse);
      }
    }
    if (IsLinkedToProxy()) {
      switch (getValueType()) {
        case vTypeString:
          delete m_Proxy.stringProxy;
          break;
        case vTypeInteger:
          delete m_Proxy.intProxy;
          break;
        case vTypeUnsignedInteger:
          delete m_Proxy.uintProxy;
          break;
        case vTypeBoolean:
          delete m_Proxy.boolProxy;
          break;
        case vTypeFloating:
          delete m_Proxy.floatingProxy;
          break;
        default:
          assert(false);
      }
      m_Proxy.iValue = 0;
      return true;
    }
    return false;
  } // unlinkProxy


  aValueType PropertyNode::getValueType() {
    return m_AliasTarget ? m_AliasTarget->m_PropVal.valueType : m_PropVal.valueType;
  } // getValueType

  void PropertyNode::setFlag(Flag _flag, bool _value) {
    checkWriteAccess();
    int oldFlags = m_Flags;
    _value ? m_Flags |= _flag : m_Flags &= ~_flag;
    if (oldFlags != m_Flags) {
      propertyChanged();
    }
  } // setFlag

  std::string PropertyNode::getAsString() {
    if (m_AliasTarget != NULL) {
      return m_AliasTarget->getAsString();
    } else {
      std::string result;

      switch (getValueType()) {
        case vTypeString:
          result = getStringValue();
          break;
        case vTypeInteger:
          result = intToString(getIntegerValue());
          break;
        case vTypeFloating:
          result = doubleToString(getFloatingValue());
          break;
        case vTypeUnsignedInteger:
          result = uintToString(getUnsignedIntegerValue());
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
    boost::recursive_mutex::scoped_lock lock(m_GlobalMutex);
    _listener->registerProperty(this);
    if (NULL == m_Listeners) {
      m_Listeners = new (std::vector<PropertyListener*>);
    }
    m_Listeners->push_back(_listener);
  } // addListener

  void PropertyNode::removeListener(PropertyListener* _listener) {
    boost::recursive_mutex::scoped_lock lock(m_GlobalMutex);
    std::vector<PropertyListener*>::iterator it = std::find(m_Listeners->begin(), m_Listeners->end(), _listener);
    if (it != m_Listeners->end()) {
      m_Listeners->erase(it);
      _listener->unregisterProperty(this);
    }
  } // removeListener

  PropertyNodePtr PropertyNode::createProperty(const std::string& _propPath) {
    checkWriteAccess();
    if (m_AliasTarget != NULL) {
      return m_AliasTarget->createProperty(_propPath);
    } else {
      if (_propPath.empty() || _propPath == "/") {
        return shared_from_this();
      }

      std::string nextOne, tail = _propPath;
      nextOne = carCdrPath(tail);
      assert(!nextOne.empty());

      PropertyNodePtr nextNode = getPropertyByName(nextOne);
      if (nextNode == NULL) {
        if (nextOne[nextOne.length() - 1] == '+') {
          nextOne.erase(nextOne.length() - 1, 1);
        }
        int index = getAndRemoveIndexFromPropertyName(nextOne);
        if (index == 0) {
          index = count(nextOne) + 1;
        }
        nextNode = boost::make_shared<PropertyNode>(nextOne.c_str(), index);
        addChild(nextNode);
      }

      return tail.empty() ? nextNode : nextNode->createProperty(tail);
    }
  } // createProperty

  bool PropertyNode::saveAsXML(std::ostream& _ofs, const int _indent, const int _flagsMask) {
    _ofs << doIndent(_indent) << "<property type=\"" << getValueTypeAsString(getValueType()) << "\"" <<
                                          " name=\"" << XMLStringEscape(getDisplayName()) << "\"";

    if (hasFlag(Archive)) {
      _ofs << " archive=\"true\"";
    }
    if (hasFlag(Readable)) {
      _ofs << " readable=\"true\"";
    }
    if (hasFlag(Writeable)) {
      _ofs << " writeable=\"true\"";
    }
    if ((m_ChildNodes && m_ChildNodes->empty()) && (getValueType() == vTypeNone)) {
      _ofs << "/>" << std::endl;
      return true;
    }

    _ofs << ">" << std::endl;

    if (getValueType() != vTypeNone) {
      _ofs << doIndent(_indent + 1) << "<value>" << XMLStringEscape(getAsString()) << "</value>" << std::endl;
    }

    bool result = true;
    if (m_ChildNodes && !m_ChildNodes->empty()) {
      result = saveChildrenAsXML(_ofs, _indent + 1, _flagsMask);
    }
    _ofs << doIndent(_indent) << "</property>" << std::endl;

    return result;
  } // saveAsXML

  bool PropertyNode::saveChildrenAsXML(std::ostream& _ofs, const int _indent, const int _flagsMask) {
    if (m_ChildNodes) {
      foreach (PropertyNodePtr pChild, *m_ChildNodes) {
        if ((_flagsMask == Flag(0)) || pChild->searchForFlag(Flag(_flagsMask))) {
          if (!pChild->saveAsXML(_ofs, _indent, _flagsMask)) {
            return false;
          }
        }
      }
    }
    return true;
  } // saveChildrenAsXML

  bool PropertyNode::searchForFlag(Flag _flag) {
    if (!hasFlag(_flag)) {
      if (m_ChildNodes) {
        foreach (PropertyNodePtr pChild, *m_ChildNodes) {
          if (pChild->searchForFlag(_flag)) {
            return true;
          }
        }
      }
      return false;
    }
    return true;
  } // searchForFlag

  void PropertyNode::propertyChanged() {
    notifyListeners(&PropertyListener::propertyChanged, shared_from_this());
  } // propertyChanged

  void PropertyNode::childAdded(PropertyNodePtr _child) {
    notifyListeners(&PropertyListener::propertyAdded, _child);
  } // childAdded

  void PropertyNode::childRemoved(PropertyNodePtr _child) {
    notifyListeners(&PropertyListener::propertyRemoved, _child);
  } // childRemoved

  void PropertyNode::notifyListeners(void (PropertyListener::*_callback)(PropertyNodePtr,PropertyNodePtr), PropertyNodePtr _node) {
    if (m_Listeners != NULL && m_Listeners->size() > 0) {
      // as the list might get modified in the listener code we need to iterate
      // over a copy of our listeners
      std::vector<PropertyListener*> copy(*m_Listeners);

      std::vector<PropertyListener*>::iterator it, lit;
      for (it = copy.begin(); it != copy.end(); it++) {

        // check if the original list still contains the listener
        lit = find(m_Listeners->begin(), m_Listeners->end(), *it);
        if (lit != m_Listeners->end() && (*lit) != NULL) {
          ((*lit)->*_callback)(shared_from_this(), _node);
        }
      }
    }

    // notify all listeners on higher levels
    if (m_ParentNode != NULL) {
      m_ParentNode->notifyListeners(_callback, _node);
    }
  } // notifyListeners

  void PropertyNode::checkWriteAccess() {
    NodePrivileges* privileges = lookupPrivileges();
    if (privileges == NULL) {
      /* no restrictions implied */
      return;
    }

    User* pUser = Security::getCurrentlyLoggedInUser();
    if (pUser == NULL) {
      throw SecurityException("Write access denied for unauthenticated user");
    }

    boost::shared_ptr<Privilege> privilege =
      privileges->getPrivilegeForRole(pUser->getRole());
    if (!privilege || !privilege->hasRight(Privilege::Write)) {
      throw SecurityException("Write access denied for user " + pUser->getName());
    }
  } // checkWriteAccess

  NodePrivileges* PropertyNode::lookupPrivileges() {
    if (m_Privileges != NULL) {
      return m_Privileges;
    }
    if (m_ParentNode != NULL) {
      return m_ParentNode->lookupPrivileges();
    }
    return NULL;
  }

  boost::recursive_mutex PropertyNode::m_GlobalMutex;

  //=============================================== PropertyParser
  PropertyParser::PropertyParser() : ExpatParser(), m_level(0),
                                     m_ignoreVersion(false),
                                     m_expectValue(false),
                                     m_ignore(false),
                                     m_currentValueType(vTypeNone)
  {};

  // this callback is triggered on each <tag>
  void PropertyParser::elementStart(const char *_name, const char **_attrs)
  {
    if (m_forceStop) {
      return;
    }

    try {
      m_currentNode->checkWriteAccess();

      // if it is the root document we need to perform some checks
      // 0 level node must be called "properties" and must have a version
      // attribute
      if (m_level == 0) {
        m_expectValue = false;
        // first node must be named "properties"
        if (strcmp(_name, "properties") != 0) {
          Logger::getInstance()->log("PropertySystem::loadFromXML: root node "
                                     "must be named properties!", lsError);
          m_forceStop = true;
          return;
        }

        if (!m_ignoreVersion) {
          // now check version
          bool versionFound = false;
          for (int i = 0; _attrs[i]; i += 2)
          {
            if (strcmp(_attrs[i], "version") == 0) {
              std::string version = _attrs[i + 1];
              if (strToIntDef(version, -1) != PROPERTY_FORMAT_VERSION) {
                Logger::getInstance()->log(std::string("PropertySystem::"
                    "loadFromXML: Version mismatch, expected ") +
                    intToString(PROPERTY_FORMAT_VERSION) + " got " + version,
                    lsError);
                m_forceStop = true;
                return;
              } else {
                versionFound = true;
              }
            }
          }
          if (!versionFound) {
            Logger::getInstance()->log("PropertySystem::loadFromXML: missing "
                                       "version attribute", lsError);
            m_forceStop = true;
            return;
          }
        }

        m_level++;
        return;
      }

      // only "property" nodes are allowed on level 1, deeper we could have
      // property or value nodes
      if (m_level == 1) {
        m_expectValue = false;
        m_currentValueType = vTypeNone;
        if (strcmp(_name, "property") != 0) {
          m_ignore = true;
          m_level++;
          return;
        } else {
          // be tolerant and simply ignore all non propety nodes on level 1
          m_ignore = false;
        }
      }

      // we are parsing a path that is being ignored, so do nothing
      if ((m_level > 1) && m_ignore)
      {
        m_level++;
        return;
      }

      // if we got here, then we are on a valid level and have valid nodes
      if (strcmp(_name, "property") == 0) {
        // first parse all propety nodes attributes
        const char* propName = NULL;
        const char* propType = NULL;
        const char* propWriteable = NULL;
        const char* propReadable = NULL;
        const char* propArchive = NULL;

        m_currentValueType = vTypeNone;

        for (int i = 0; _attrs[i]; i += 2)
        {
          if (strcmp(_attrs[i], "name") == 0) {
            propName = _attrs[i + 1];
          } else if (strcmp(_attrs[i], "type") == 0) {
            propType = _attrs[i + 1];
          } else if (strcmp(_attrs[i], "writable") == 0) {
            propWriteable = _attrs[i + 1];
          } else if (strcmp(_attrs[i], "readable") == 0) {
            propReadable = _attrs[i + 1];
          } else if (strcmp(_attrs[i], "archive") == 0) {
            propArchive = _attrs[i + 1];
          }
        }

        // property name is mandatory
        if (propName == NULL) {
          Logger::getInstance()->log("PropertySystem::loadFromXML: property "
                                     "tag is missing the name attribute",
                                     lsError);
          m_forceStop = true;
          return;
        }

        if (propType != NULL) {
          m_currentValueType = getValueTypeFromString(propType);
        } else {
          m_currentValueType = vTypeNone;
        }

        // ok this is a special "proprety system" feature,basically one can have
        // node names that describe full paths, i.e. /path/to/node
        // for the above case only the last element is supposed to have an
        // associated value, the rest is just a path to the node. since that
        // path is not part of the actual XML we will have to take care of
        // generating this structure ourselves
        //
        // in theory the createProperty() method of the ProprtyNode class should
        // be able to take care of this, but it simply did not work in the way
        // I would have expected, the resulting tree structure became messed up.
        // so we'll be taking care of it manually here

        PropertyNodePtr node;
        PropertyNodePtr temp = m_currentNode;
        PropertyNodePtr path;

        const char *slash = strchr(propName, '/');
        const char *start = propName;
        do {
          // try to get first part of the node path
          std::string part;
          if (slash != NULL) {
            part = std::string(start, slash - start);
            start = slash + 1;
            if (start != NULL) {
              slash = strchr(start, '/');
            }
          } else {
            part = start;
            start = NULL;
          }
          path.reset();

          part = part.substr(0, part.find('['));

          // this is important, if a node with the same name already exists we
          // have to reuse it, otherwise we will generate an array wich is
          // not what we want. root level requires additional handling, this is
          // what the property system seems to expect
          boost::recursive_mutex::scoped_lock lock(PropertyNode::m_GlobalMutex);
          if ((m_level == 1) && (m_currentNode->getName() == part)) {
            path = m_currentNode;
          } else {
            path = temp->getPropertyByName(part);
          }
          if (path == NULL) {
            path = PropertyNodePtr(new PropertyNode(part.c_str()));
            temp->addChild(path);
          }
          temp = path;
        } while (start != NULL);
        node = path;

        if (propWriteable != NULL)
        {
          node->setFlag(PropertyNode::Writeable,
                        strcmp(propWriteable, "true") == 0);
        }
        if (propReadable != NULL)
        {
          node->setFlag(PropertyNode::Readable,
                        strcmp(propReadable, "true") == 0);
        }
        if (propArchive != NULL)
        {
          node->setFlag(PropertyNode::Archive, strcmp(propArchive, "true") == 0);
        }

        // we're not pushing the interim nodes that may have been created above
        // because the XML does not know anything about the weirdo-property
        // tree structure
        m_nodes.push(m_currentNode);
        m_currentNode = node;
      } else if (strcmp(_name, "value") == 0)  { // we encountered a value tag
        if (m_currentValueType == vTypeNone) {
          Logger::getInstance()->log("PropertySystem::loadFromXML: missing "
                                     "value type for property", lsError);
          m_forceStop = true;
          return;
        }
        // tell the character data callback that it should do something useful
        m_expectValue = true;
      }

      m_level++;
    } catch (std::runtime_error& ex) {
      m_forceStop = true;
      Logger::getInstance()->log(std::string("PropertySystem::loadFromXML: ") +
              "element start handler caught exception: " + ex.what() +
              " Will abort parsing!", lsError);
    } catch (...) {
      m_forceStop = true;
      Logger::getInstance()->log("PropertySystem::loadFromXML: "
              "element start handler caught exception! Will abort parsing!",
              lsError);
    }
  }

  // this callback is triggered for each </tag>
  void PropertyParser::elementEnd(const char *_name) {
    if (m_forceStop) {
      return;
    }

    m_level--;
    // this should NEVER happen and is therefore a good check/test to
    // detect bugs/error conditions that remained unhandled; I know that
    // rest of the proptree classes uses asserts here and there but that's
    // not really nice, so I prefer to tell the calling function that the
    // XML could not be parsed, instead of just crashing ;)
    if (m_level < 0) {
        Logger::getInstance()->log("PropertySystem::loadFromXML: elementEnd "
                                   "invalid document depth!", lsError);
        m_forceStop = true;
        return;
    }

    // we can't have several value tags one after the other, so if we
    // got into a <value> then we do not expect any further <value> tags
    m_expectValue = false;

    // if we got into an "unsupported" branch, i.e. level 1 was not a <property>
    // then we simply ignore it
    if (m_ignore) {
      return;
    }

    try {
      // handle position in the document, we only put property nodes into the
      // stack
      if (strcmp(_name, "property") == 0) {
        m_currentNode = m_nodes.top();
        m_nodes.pop();
        m_currentValueType = vTypeNone;
      } else if (strcmp(_name, "value") == 0) {
        switch (m_currentValueType) {
          case vTypeString:
            // we may receive one string through two callbacks depending when
            // expat hits the buffer boundary
            m_currentNode->setStringValue(m_temporaryValue);
            // reset flag if we were able to set a string value
            break;
          case vTypeInteger:
            m_currentNode->setIntegerValue(strToInt(m_temporaryValue));
            break;
          case vTypeUnsignedInteger:
            m_currentNode->setUnsignedIntegerValue(strToUInt(m_temporaryValue));
            break;
          case vTypeBoolean:
            m_currentNode->setBooleanValue(m_temporaryValue == "true");
            break;
          case vTypeFloating:
            {
              double value = ::strtod(m_temporaryValue.c_str(), 0);
              m_currentNode->setFloatingValue(value);
            }
            break;
          default:
            Logger::getInstance()->log("PropertySystem::loadFromXML: character "
                                       "data callback - unknown value type!");
            m_forceStop = true;
        }
      }
      m_temporaryValue.clear();
    } catch (std::runtime_error& ex) {
      m_forceStop = true;
      Logger::getInstance()->log(std::string("PropertySystem::loadFromXML: ") +
              "element end handler caught exception: " + ex.what() +
              " Will abort parsing!", lsError);
    } catch (...) {
      m_forceStop = true;
      Logger::getInstance()->log("PropertySystem::loadFromXML: "
              "element end handler caught exception! Will abort parsing!",
              lsError);

    }
  }

  // this callback gets triggered when we receive some character data between
  // tags, i.e.: <tag>character data</tag>
  void PropertyParser::characterData(const XML_Char *_s, int _len) {
    if (m_forceStop) {
      return;
    }

    // the property system only supports character data enclosed in a <value>
    // tag, everything else is ignored
    if (!m_expectValue) {
      return;
    }

    if (m_currentValueType == vTypeNone) {
        Logger::getInstance()->log("PropertySystem::loadFromXML: character "
                                   "data callback - received value data but "
                                   "value type is missing, ignoring");
        return;
    }

    std::string value = std::string(_s, _len);
    // the empty check is probably not needed, if I understand expat correctly
    // we won't get called if there is no data available, still better be safe
    // than sorry
    if (!value.empty() && (m_currentValueType != vTypeNone))
    {
      m_temporaryValue += value;
    }
  }

  void PropertyParser::clearStack()
  {
    while (!m_nodes.empty()) {
      m_nodes.pop();
    }
  }

  void PropertyParser::reinitMembers(PropertyNodePtr _node,
                                     bool _ignoreVersion) {
    // the propety parser class instance can be reused, so we will reset
    // the internals on each call of the loadFromXML function
    m_level = 0;
    m_ignoreVersion = _ignoreVersion;
    m_expectValue = false;
    m_ignore = false;
    m_currentValueType = vTypeNone;
    m_currentNode = _node;
    m_temporaryValue.clear();
    clearStack();

    m_nodes.push(_node);
    m_currentNode = _node;
  }

  bool PropertyParser::loadFromXML(const std::string& _fileName,
                                   PropertyNodePtr _node, bool _ignoreVersion) {
    if (_node == NULL) {
        return false;
    }

    reinitMembers(_node, _ignoreVersion);

    bool ret = parseFile(_fileName);
    clearStack();
    return ret;
  } // loadFromXML


  PropertyParserProxy::PropertyParserProxy() : PropertyParser() {}
  
  void PropertyParserProxy::elementStartCb(const char *_name,
                                           const char **_attrs) {
    elementStart(_name, _attrs);
  }
  
  void PropertyParserProxy::elementEndCb(const char *_name) {
    elementEnd(_name);
  }
 
  void PropertyParserProxy::characterDataCb(const XML_Char *_s, int _len) {
    characterData(_s, _len);
  }

  void PropertyParserProxy::reset(PropertyNodePtr _node, bool _ignoreVersion) {
    reinitMembers(_node, _ignoreVersion);
  }
  
  //=============================================== PropertyListener

  PropertyListener::~PropertyListener() {
    unsubscribe();
  } // dtor

  void PropertyListener::unsubscribe() {
    // while this does look like an infinite loop it isn't
    // as the property will call unregisterProperty in removeListener
    while (!m_Properties.empty()) {
      m_Properties.front()->removeListener(this);
    }
  } // unsubscribe

  void PropertyListener::propertyChanged(PropertyNodePtr _caller, PropertyNodePtr _changedNode) {
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
    if (it != m_Properties.end()) {
      m_Properties.erase(it);
    }
  } // unregisterProperty


  //=============================================== Utilities

  const char* getValueTypeAsString(aValueType _value) {
    switch(_value) {
      case vTypeNone:
        return "none";
        break;
      case vTypeInteger:
        return "integer";
        break;
      case vTypeUnsignedInteger:
        return "unsignedinteger";
        break;
      case vTypeString:
        return "string";
        break;
      case vTypeBoolean:
        return "boolean";
        break;
      case vTypeFloating:
        return "floating";
        break;
    }
    return "unknown";
  } // getValueTypeAsString

  aValueType getValueTypeFromString(const char* _str) {
    std::string strVal = _str;
    if (strVal == "none") {
      return vTypeNone;
    } else if (strVal == "integer") {
      return vTypeInteger;
    } else if (strVal == "unsignedinteger") {
      return vTypeUnsignedInteger;
    } else if (strVal == "boolean") {
      return vTypeBoolean;
    } else if (strVal == "string") {
      return vTypeString;
    } else if (strVal == "floating") {
      return vTypeFloating;
    } else {
      assert(false);
    }
  } // getValueTypeFromString


  //==================================================== Default property values

  template <> const int PropertyProxy<int>::DefaultValue = 0;
  template <> const bool PropertyProxy<bool>::DefaultValue = false;
  template <> const std::string PropertyProxy<std::string>::DefaultValue = std::string();
  template <> const double PropertyProxy<double>::DefaultValue = 0.0;

}
