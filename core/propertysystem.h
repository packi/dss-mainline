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

#ifndef NEUROPROPERTYSYSTEM_H
#define NEUROPROPERTYSYSTEM_H

#include <libxml/tree.h>
#include <libxml/xmlwriter.h>

#include <boost/mpl/if.hpp>
#include <boost/type_traits/is_class.hpp>

#include <stdexcept>
#include <vector>
#include <string>

namespace dss {

  class PropertyNode;

  typedef enum {
    vTypeNone = 0, vTypeInteger, vTypeString, vTypeBoolean
  } aValueType;

  const char* getValueTypeAsString(aValueType _value);
  aValueType getValueTypeFromString(const char* _str);

  typedef struct {
    aValueType valueType;
    union {
      char* pString;
      int integer;
      bool boolean;
    } actualValue;
    int PropertyLevel;
  } aPropertyValue;

  std::string getBasePath(const std::string& _path);
  std::string getProperty(const std::string& _path);
  std::string getRoot(const std::string& _path);

  /** A tree tree consisting of different value nodes.
   * The tree can by serialized to and from XML. Nodes can either
   * store values by themselves or, with the use of proxies be
   * linked directly to an object or pointer.
   * @author Patrick Staehlin <me@packi.ch>
   */
  class PropertySystem {
  private:
    PropertyNode* m_RootNode;
  public:
    PropertySystem();
    ~PropertySystem();

    bool loadFromXML(const std::string& _fileName, PropertyNode* _rootNode = NULL);
    bool saveToXML(const std::string& _fileName, PropertyNode* _rootNode = NULL) const;

    PropertyNode* getProperty(const std::string& _propPath) const;

    PropertyNode* getRootNode() const {
      return m_RootNode;
    }
    ;

    PropertyNode* createProperty(const std::string& _propPath);

    // fast access to property values
    int getIntValue(const std::string& _propPath) const;
    bool getBoolValue(const std::string& _propPath) const;
    std::string getStringValue(const std::string& _propPath) const;

    bool setIntValue(const std::string& _propPath, const int _value,
                     bool _mayCreate = true, bool _mayOverwrite = true);
    bool setBoolValue(const std::string& _propPath, const bool _value,
                      bool _mayCreate = true, bool _mayOverwrite = true);
    bool setStringValue(const std::string& _propPath, const std::string& _value,
                        bool _mayCreate = true, bool _mayOverwrite = true);
  }; //  PropertySystem


  template<class T>
  class PropertyProxy {
  public:
    virtual ~PropertyProxy() {
    }
    ;

    static const T DefaultValue;
    virtual T getValue() const = 0;
    virtual void setValue(T _value) = 0;

    virtual PropertyProxy* clone() const = 0;
  }; // PropertyProxy


  template<class T>
  class PropertyProxyReference : public PropertyProxy<T> {
  private:
    T& m_Reference;
    bool m_Writeable;
  public:
    PropertyProxyReference(T& _reference, bool _writeable = true)
    : m_Reference(_reference),
      m_Writeable(_writeable)
    { }

    virtual ~PropertyProxyReference() { }

    virtual T getValue() const { return m_Reference; }

    virtual void setValue(T _value) {
      if(m_Writeable) {
        m_Reference = _value;
      }
    }

    virtual PropertyProxyReference* clone() const {
      return new PropertyProxyReference<T>(m_Reference, m_Writeable);
    }
  };

  template<class T>
  class PropertyProxyPointer: public PropertyProxy<T> {
  private:
    T* m_PointerToValue;
  public:
    PropertyProxyPointer(T* _ptrToValue)
    : m_PointerToValue(_ptrToValue)
    { }

    virtual ~PropertyProxyPointer() { }

    virtual T getValue() const {
      return *m_PointerToValue;
    }
    ;
    virtual void setValue(T _value) {
      *m_PointerToValue = _value;
    }
    ;

    virtual PropertyProxyPointer* clone() const {
      return new PropertyProxyPointer<T> (m_PointerToValue);
    }
  }; // PropertyProxyPointer


  template<class T>
  class PropertyProxyStaticFunction: public PropertyProxy<T> {
  private:
    typedef T (*aGetterType)();
    typedef void (*aSetterType)(T);
    aGetterType m_GetterPtr;
    aSetterType m_SetterPtr;
  public:
    PropertyProxyStaticFunction(aGetterType _getter = 0, aSetterType _setter = 0)
    : m_GetterPtr(_getter),
      m_SetterPtr(_setter)
    { }

    virtual ~PropertyProxyStaticFunction()
    { }

    virtual T getValue() const {
      if (m_GetterPtr != NULL) {
        return (*m_GetterPtr)();
      } else {
        return PropertyProxy<T>::DefaultValue;
      }
    } // GetValue

    virtual void setValue(T _value) {
      if (m_SetterPtr != NULL) {
        (*m_SetterPtr)(_value);
      }
    } // SetValue

    virtual PropertyProxyStaticFunction* clone() const {
      return new PropertyProxyStaticFunction<T> (m_GetterPtr, m_SetterPtr);
    }
  }; // PropertyProxyStaticFunction


  template<class Cls, class T>
  class PropertyProxyMemberFunction: public PropertyProxy<T> {
  private:
    typedef typename boost::mpl::if_c<boost::is_class<T>::value, const T&, T>::type (Cls::*aGetterType)() const;
    typedef void (Cls::*aSetterType)(typename boost::mpl::if_c<boost::is_class<T>::value, const T&, const T>::type);
    Cls* m_Obj;
    const Cls* m_ConstObj;
    aGetterType m_GetterPtr;
    aSetterType m_SetterPtr;
  public:
    PropertyProxyMemberFunction(Cls& _obj, aGetterType _getter = NULL,
                                aSetterType _setter = NULL)
    : m_Obj(&_obj), m_ConstObj(NULL), m_GetterPtr(_getter), m_SetterPtr(_setter)
    { }

    PropertyProxyMemberFunction(const Cls& _obj, aGetterType _getter = NULL)
    : m_Obj(NULL), m_ConstObj(&_obj), m_GetterPtr(_getter), m_SetterPtr(NULL)
    { }


    virtual ~PropertyProxyMemberFunction() {
    }

    virtual T getValue() const {
      if (m_GetterPtr != NULL) {
        if(m_Obj != NULL) {
          return (*m_Obj.*m_GetterPtr)();
        } else if(m_ConstObj != NULL) {
          return (*m_ConstObj.*m_GetterPtr)();
        }
      }
      return PropertyProxy<T>::DefaultValue;
    } // GetValue

    virtual void setValue(T _value) {
      if (m_SetterPtr != NULL && m_Obj != NULL) {
        (*m_Obj.*m_SetterPtr)(_value);
      }
    } // SetValue

    virtual PropertyProxyMemberFunction* clone() const {
      if(m_Obj != NULL) {
        return new PropertyProxyMemberFunction<Cls, T> (*m_Obj, m_GetterPtr,
                                                        m_SetterPtr);
      } else {
        return new PropertyProxyMemberFunction<Cls, T> (*m_ConstObj, m_GetterPtr);
      }
    }
  }; // PropertyProxyMemberFunction


  class PropertyListener {
  private:
    std::vector<const PropertyNode*> m_Properties;
  protected:
    friend class PropertyNode;
    virtual void propertyChanged(const PropertyNode* _changedNode);

    void registerProperty(const PropertyNode* _node);
    void unregisterProperty(const PropertyNode* _node);
  public:
    virtual ~PropertyListener();
  }; // PropertyListener


  class PropertyNode {
  private:
    aPropertyValue m_PropVal;
    union {
      PropertyProxy<bool>* boolProxy;
      PropertyProxy<int>* intProxy;
      PropertyProxy<std::string>* stringProxy;
    } m_Proxy;
    std::vector<PropertyNode*> m_ChildNodes;
    mutable std::vector<PropertyListener*> m_Listeners;
    PropertyNode* m_ParentNode;
    std::string m_Name;
    mutable std::string m_DisplayName;
    bool m_LinkedToProxy;
    int m_Index;
  private:
    void addChild(PropertyNode* _childNode);
    void clearValue();

    int getAndRemoveIndexFromPropertyName(std::string& _propName);

    void notifyListeners(void(PropertyListener::*_callback)(const PropertyNode*));
  public:
    PropertyNode(PropertyNode* _parentNode, const char* _name, int _index = 0);
    ~PropertyNode();

    const std::string& getName() const {
      return m_Name;
    }
    const std::string& getDisplayName() const;

    PropertyNode* getProperty(const std::string& _propPath);
    int count(const std::string& _propertyName);

    void setStringValue(const char* _value);
    void setStringValue(const std::string& _value);
    void setIntegerValue(const int _value);
    void setBooleanValue(const bool _value);

    std::string getStringValue();
    int getIntegerValue();
    bool getBoolValue();

    std::string getAsString();

    PropertyNode* createProperty(const std::string& _propPath);
    void moveTo(const std::string& _path);

    PropertyNode* getPropertyByName(const std::string& _name);

    aValueType getValueType();

    // proxy support
    bool linkToProxy(const PropertyProxy<bool>& _proxy);
    bool linkToProxy(const PropertyProxy<int>& _proxy);
    bool linkToProxy(const PropertyProxy<std::string>& _proxy);
    bool unlinkProxy();

    void addListener(PropertyListener* _listener) const;
    void removeListener(PropertyListener* _listener) const;

    int getChildCount() const { return m_ChildNodes.size(); }
    PropertyNode* getChild(const int _index) { return m_ChildNodes.at(_index); }

    void foreachChildOf(void(*_callback)(PropertyNode*)) {
      for (std::vector<PropertyNode*>::iterator it = m_ChildNodes.begin(); it
          != m_ChildNodes.end(); ++it) {
        (*_callback)(*it);
      }
    } // ForeachChildOf

    // needs to stay inside the header file, else the template specializations won't get generated ;-=
    template<class Cls>
    void foreachChildOf(Cls& _objRef, void(Cls::*_callback)(PropertyNode*)) {
      for (std::vector<PropertyNode*>::iterator it = m_ChildNodes.begin(); it
          != m_ChildNodes.end(); ++it) {
        (_objRef.*_callback)(*it);
      }
    }
  public:
    bool saveAsXML(xmlTextWriterPtr _writer);
    bool loadFromNode(xmlNode* _pNode);
  }; // PropertyNode

  class PropertyTypeMismatch : public std::runtime_error {
  public:
    explicit
    PropertyTypeMismatch(const std::string&  __arg)
    : runtime_error(__arg)
    { }
  };

} // namespace dss

#endif
