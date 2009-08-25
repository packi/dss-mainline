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
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

namespace dss {

  class PropertyNode;
  typedef boost::shared_ptr<PropertyNode> PropertyNodePtr;

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
    PropertyNodePtr m_RootNode;
  public:
    PropertySystem();
    ~PropertySystem();

    /** Loads a subtree from XML.
     * Everything in the XML has to be relatove to _rootNode. */
    bool loadFromXML(const std::string& _fileName, PropertyNodePtr _rootNode);
    /** Saves a subtree to XML. */
    bool saveToXML(const std::string& _fileName, PropertyNodePtr _rootNode) const;

    /** Searches a property by path.
     * @return The node, or NULL if not found. */
    PropertyNodePtr getProperty(const std::string& _propPath) const;

    /** Returns the root node. */
    PropertyNodePtr getRootNode() const {
      return m_RootNode;
    }
    ;

    /** Creates a property and the path to it. */
    PropertyNodePtr createProperty(const std::string& _propPath);

    // fast access to property values
    /** Returns the value of a property as an int.
     * Will throw an exception if the node does not exist or
     * the types don't match.*/
    int getIntValue(const std::string& _propPath) const;
    /** Returns the value of a property as a boolean.
     * Will throw an exception if the node does not exist or
     * the types don't match.*/
    bool getBoolValue(const std::string& _propPath) const;
    /** Returns the value of a property as a string.
     * Will throw an exception if the node does not exist or
     * the types don't match.*/
    std::string getStringValue(const std::string& _propPath) const;

    /** Sets the value of a property.
     * @return \c true if the property was successfully set.
     * @param _mayCreate If \c true, the property gets created if it does not exist
     * @param _mayOverwrite If \c false, the property does not get set if it already exists */
    bool setIntValue(const std::string& _propPath, const int _value,
                     bool _mayCreate = true, bool _mayOverwrite = true);
    /** Sets the value of a property.
     * @return \c true if the property was successfully set.
     * @param _mayCreate If \c true, the property gets created if it does not exist
     * @param _mayOverwrite If \c false, the property does not get set if it already exists */
    bool setBoolValue(const std::string& _propPath, const bool _value,
                      bool _mayCreate = true, bool _mayOverwrite = true);
    /** Sets the value of a property.
     * @return \c true if the property was successfully set.
     * @param _mayCreate If \c true, the property gets created if it does not exist
     * @param _mayOverwrite If \c false, the property does not get set if it already exists */
    bool setStringValue(const std::string& _propPath, const std::string& _value,
                        bool _mayCreate = true, bool _mayOverwrite = true);
  }; //  PropertySystem


  /** Abstract base class for property proxies.
   * A property-proxy can be linked to a property node. By
   * specifying a proxy, the node will get a fixed type T.
   * Calls to set or get the value of the linked node
   * will be redirected to the proxy. */
  template<class T>
  class PropertyProxy {
  public:
    virtual ~PropertyProxy() {
    }

    /** Default value for write-only properties. */
    static const T DefaultValue;
    /** Returns the value of the property. */
    virtual T getValue() const = 0;
    /** Sets the value for a property. */
    virtual void setValue(T _value) = 0;

    /** Creates a copy of the proxy. */
    virtual PropertyProxy* clone() const = 0;
  }; // PropertyProxy


  /** PropertyProxy that is links to a reference of a variable. */
  template<class T>
  class PropertyProxyReference : public PropertyProxy<T> {
  private:
    T& m_Reference;
    bool m_Writeable;
  public:
    /** Constructs a PropertyProxyReference.
     * @param _reference The reference it gets linked to
     * @param _writeable If \c true (default) values will get written back to _reference*/
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

  /** PropertyProxy that links to a pointer of a variable. */
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


  /** PropertyProxy that links to a static methods or simple functions. */
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


  /** PropertyProxy that links to a member function.
   * Cls is the type of the class that we're linking against. */
  template<class Cls, class T>
  class PropertyProxyMemberFunction: public PropertyProxy<T> {
  private:
    // magic ahead: if the type of T is a class, expect a const-reference, else expect a copy of the value
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


  /** Notifies a class if one or many properties change. */
  class PropertyListener {
  private:
    std::vector<PropertyNode*> m_Properties;
  protected:
    friend class PropertyNode;
    /** Function that gets called if a property changes. */
    virtual void propertyChanged(PropertyNodePtr _changedNode);
    virtual void propertyRemoved(PropertyNodePtr _parent, PropertyNodePtr _child);
    virtual void propertyAdded(PropertyNodePtr _parent, PropertyNodePtr _child);

    /** Add a property node to the notifiers. */
    void registerProperty(PropertyNodePtr _node);
    /** Remove a property from the notifiers */
    void unregisterProperty(PropertyNodePtr _node);
  public:
    virtual ~PropertyListener();
  }; // PropertyListener


  typedef std::vector<PropertyNodePtr> PropertyList;

  /** The heart of the PropertySystem. */
  class PropertyNode : public boost::enable_shared_from_this<PropertyNode> {
  private:
    aPropertyValue m_PropVal;
    union {
      PropertyProxy<bool>* boolProxy;
      PropertyProxy<int>* intProxy;
      PropertyProxy<std::string>* stringProxy;
    } m_Proxy;
    PropertyList m_ChildNodes;
    std::vector<PropertyNode*> m_AliasedBy;
    std::vector<PropertyListener*> m_Listeners;
    PropertyNode* m_ParentNode;
    std::string m_Name;
    mutable std::string m_DisplayName;
    bool m_LinkedToProxy;
    bool m_Aliased;
    PropertyNode* m_AliasTarget;
    int m_Index;
  private:
    void clearValue();

    int getAndRemoveIndexFromPropertyName(std::string& _propName);

    void childAdded(PropertyNodePtr _child);
    void childRemoved(PropertyNodePtr _child);
    void notifyListeners(void(PropertyListener::*_callback)(PropertyNodePtr));
    void notifyListeners(void(PropertyListener::*_callback)(PropertyNodePtr, PropertyNodePtr), PropertyNodePtr _node);
  public:
    PropertyNode(const char* _name, int _index = 0);
    ~PropertyNode();

    /** Returns the name of the property. */
    const std::string& getName() const {
      return m_Name;
    }
    /** Returns the displayname of the property. Since a PropertyNode can have
     * multiple subnodes, the displayname includes the index.*/
    const std::string& getDisplayName() const;

    /** Returns a child node by path.
     * @return The child or NULL if not found */
    PropertyNodePtr getProperty(const std::string& _propPath);
    int count(const std::string& _propertyName);

    /** Sets the value as string. */
    void setStringValue(const char* _value);
    /** Sets the value as string. */
    void setStringValue(const std::string& _value);
    /** Sets the value as integer. */
    void setIntegerValue(const int _value);
    /** Sets the value as boolean. */
    void setBooleanValue(const bool _value);

    /** Returns the string value.
     * Throws an exception if the value-types don't match. */
    std::string getStringValue();
    /** Returns the integer value.
     * Throws an exception if the value-types don't match. */
    int getIntegerValue();
    /** Returns the boolean value.
     * Throws an exception if the value-types don't match. */
    bool getBoolValue();

    /** Returns the value as string, regardless of it's value-type.*/
    std::string getAsString();

    /** Recursively adds a child-node. */
    PropertyNodePtr createProperty(const std::string& _propPath);
    /** Moves this node to \a _path. */
    void moveTo(const std::string& _path);

    /** Returns a child node by name.
     * Or NULL if not found.*/
    PropertyNodePtr getPropertyByName(const std::string& _name);

    /** Returns the type of the property. */
    aValueType getValueType();

    void alias(PropertyNodePtr _target);

    // proxy support
    /** Links to the given proxy.
     * The incoming proxy will get cloned. */
    bool linkToProxy(const PropertyProxy<bool>& _proxy);
    /** @copydoc linkToProxy */
    bool linkToProxy(const PropertyProxy<int>& _proxy);
    /** @copydoc linkToProxy */
    bool linkToProxy(const PropertyProxy<std::string>& _proxy);
    /** Unlinks from a proxy */
    bool unlinkProxy();

    /** Adds a listener. */
    void addListener(PropertyListener* _listener);
    /** Removes a listener */
    void removeListener(PropertyListener* _listener);

    /** Returns the count of the nodes children. */
    int getChildCount() const { return m_ChildNodes.size(); }
    /** Returns a child node by index. */
    PropertyNodePtr getChild(const int _index) { return m_ChildNodes.at(_index); }

    void addChild(PropertyNodePtr _childNode);
    PropertyNodePtr removeChild(PropertyNodePtr _childNode);

    /** Notifies all listeners that the value of this property has changed */
    void propertyChanged();

    /** Performs \a _callback for each child node (non-recursive) */
    void foreachChildOf(void(*_callback)(PropertyNode&)) {
      for (PropertyList::iterator it = m_ChildNodes.begin(); it
          != m_ChildNodes.end(); ++it) {
        (*_callback)(**it);
      }
    } // ForeachChildOf

    // needs to stay inside the header file, else the template specializations won't get generated ;-=
    /** @copydoc foreachChildOf */
    template<class Cls>
    void foreachChildOf(Cls& _objRef, void(Cls::*_callback)(PropertyNode&)) {
      for (PropertyList::iterator it = m_ChildNodes.begin(); it
          != m_ChildNodes.end(); ++it) {
        (_objRef.*_callback)(**it);
      }
    }
  public:
    /** Writes the node to XML */
    bool saveAsXML(xmlTextWriterPtr _writer);
    /** Loads the node from XML */
    bool loadFromNode(xmlNode* _pNode);
  }; // PropertyNode

  /** Exception that gets thrown if a incompatible assignment would take place. */
  class PropertyTypeMismatch : public std::runtime_error {
  public:
    explicit
    PropertyTypeMismatch(const std::string&  __arg)
    : runtime_error(__arg)
    { }
  };

} // namespace dss

#endif
