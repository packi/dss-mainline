/*
    Copyright (c) 2009,2010,2012 digitalSTROM.org, Zurich, Switzerland
    Copyright (c) 2008 Patrick Staehlin <me@packi.ch>

    Authors: Patrick Staehlin, futureLAB AG <pstaehlin@futurelab.ch>
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

#ifndef NEUROPROPERTYSYSTEM_H
#define NEUROPROPERTYSYSTEM_H

#include <vector>
#include <string>

#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/flyweight.hpp>

#include "src/logger.h"

namespace boost {
class recursive_mutex;
} // namespace boost

namespace dss {

  class PropertyNode;
  typedef boost::shared_ptr<PropertyNode> PropertyNodePtr;

  typedef enum {
    vTypeNone = 0, vTypeInteger, vTypeString, vTypeBoolean, vTypeFloating,
    vTypeUnsignedInteger
  } aValueType;

  const char* getValueTypeAsString(aValueType _value);
  aValueType getValueTypeFromString(const char* _str);

  typedef struct {
    aValueType valueType;
    union {
      char* pString;
      int integer;
      uint32_t unsignedinteger;
      bool boolean;
      double floating;
    } actualValue;
  } aPropertyValue;

  /**
   * Saves a subtree to XML.
   * @param _rootNode
   */
  bool saveToXML(const std::string& _fileName, PropertyNodePtr _rootNode,
                 const int _flagsMask = 0);
  /**
   * Loads a subtree from XML.
   * @param _rootNode -- content of the XML is appended to the _rootNode. */
  bool loadFromXML(const std::string& _fileName, PropertyNodePtr _rootNode);

  /** A tree tree consisting of different value nodes.
   * The tree can by serialized to and from XML. Nodes can either
   * store values by themselves or, with the use of proxies be
   * linked directly to an object or pointer.
   * @author Patrick Staehlin <me@packi.ch>
   */
  class PropertySystem : boost::noncopyable {
    __DECL_LOG_CHANNEL__
  private:
    PropertyNodePtr m_RootNode;
  public:
    PropertySystem();
    ~PropertySystem();

    /** Searches a property by path.
     * @return The node, or NULL if not found. */
    PropertyNodePtr getProperty(const std::string& _propPath) const;

    /** Returns the root node. */
    PropertyNodePtr getRootNode() const {
      return m_RootNode;
    }

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
  class PropertyProxy : boost::noncopyable {
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
  template<class T, class ReferenceType = T>
  class PropertyProxyReference : public PropertyProxy<T> {
  private:
    ReferenceType& m_Reference;
    bool m_Writeable;
  public:
    /** Constructs a PropertyProxyReference.
     * @param _reference The reference it gets linked to
     * @param _writeable If \c true (default) values will get written back to _reference*/
    PropertyProxyReference(ReferenceType& _reference, bool _writeable = true)
    : m_Reference(_reference),
      m_Writeable(_writeable)
    { }

    virtual ~PropertyProxyReference() { }

    virtual T getValue() const { return static_cast<T>(m_Reference); }

    virtual void setValue(T _value) {
      if(m_Writeable) {
        m_Reference = static_cast<ReferenceType>(_value);
      }
    }

    virtual PropertyProxyReference* clone() const {
      return new PropertyProxyReference<T, ReferenceType>(m_Reference, m_Writeable);
    }
  };

  template<class ReferenceType>
  class PropertyProxyToString : public PropertyProxy<std::string> {
  private:
    ReferenceType& m_Reference;
  public:
    /** Constructs a PropertyProxyReference.
     * @param _reference The reference it gets linked to
     */
    PropertyProxyToString(ReferenceType& _reference)
    : m_Reference(_reference)
    { }

    virtual ~PropertyProxyToString() { }
    virtual std::string getValue() const { return toString(m_Reference); }
    virtual void setValue(std::string _value) { /* read-only */ }

    virtual PropertyProxyToString* clone() const {
      return new PropertyProxyToString<ReferenceType>(m_Reference);
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
  template<class Cls, class T, bool constIfClass = true, bool constGetter = true>
  class PropertyProxyMemberFunction: public PropertyProxy<T> {
  private:
    // magic ahead: if the type of T is a class, expect a const-reference (unless constIfClass is false), else expect a copy of the value
    typedef typename boost::mpl::if_c<boost::mpl::and_<boost::is_class<T>, boost::mpl::bool_<constIfClass> >::value, const T&, T>::type aConvertedValueType;
    typedef aConvertedValueType (Cls::*aConstGetterType)() const;
    typedef aConvertedValueType (Cls::*aNonConstGetterType)();
    typedef typename boost::mpl::if_c<constGetter, aConstGetterType, aNonConstGetterType>::type aGetterType;
    typedef void (Cls::*aSetterType)(aConvertedValueType);
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
    } // getValue

    virtual void setValue(T _value) {
      if (m_SetterPtr != NULL && m_Obj != NULL) {
        (*m_Obj.*m_SetterPtr)(_value);
      }
    } // getValue

    virtual PropertyProxyMemberFunction* clone() const {
      if(m_Obj != NULL) {
        return new PropertyProxyMemberFunction<Cls, T, constIfClass, constGetter> (*m_Obj, m_GetterPtr,
                                                        m_SetterPtr);
      } else {
        return new PropertyProxyMemberFunction<Cls, T, constIfClass, constGetter> (*m_ConstObj, m_GetterPtr);
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
    virtual void propertyChanged(PropertyNodePtr _caller, PropertyNodePtr _changedNode);
    virtual void propertyRemoved(PropertyNodePtr _parent, PropertyNodePtr _child);
    virtual void propertyAdded(PropertyNodePtr _parent, PropertyNodePtr _child);

    /** Add a property node to the notifiers. */
    void registerProperty(PropertyNode* _node);
    /** Remove a property from the notifiers */
    void unregisterProperty(PropertyNode* _node);
  public:
    bool isListening() { return !m_Properties.empty(); }
    void unsubscribe();
    virtual ~PropertyListener();
  }; // PropertyListener


  typedef std::vector<PropertyNodePtr> PropertyList;
  class NodePrivileges;
  class Privilege;

  /** The heart of the PropertySystem. */
  class PropertyNode : boost::noncopyable, public boost::enable_shared_from_this<PropertyNode> {
    __DECL_LOG_CHANNEL__

  public:
    enum Flag {
      Readable = 1 << 0, /**< Node is readable */
      Writeable = 1 << 1, /**< Node is writeable */
      Archive = 1 << 2 /**< Node will get written to XML (hint only) */
    };

    static boost::recursive_mutex m_GlobalMutex;

  private:                                  /* Size: 32 64 bit */
    aPropertyValue m_PropVal;                     /*  8  8 */
    union {
      PropertyProxy<bool>* boolProxy;
      PropertyProxy<int>* intProxy;
      PropertyProxy<uint32_t>* uintProxy;
      PropertyProxy<std::string>* stringProxy;
      PropertyProxy<double>* floatingProxy;
      int iValue;
    } m_Proxy;                                    /*  4  8 */
    boost::flyweight<std::string> m_Name;         /*  4  8 */
    mutable std::string m_DisplayName;            /*  4  8 */
    std::vector<PropertyNodePtr>* m_ChildNodes;   /*  4  8 */
    std::vector<PropertyNode*>* m_AliasedBy;      /*  4  8 */
    std::vector<PropertyListener*>* m_Listeners;  /*  4  8 */
    PropertyNode* m_ParentNode;                   /*  4  8 */
    PropertyNode* m_AliasTarget;                  /*  4  8 */
    NodePrivileges* m_Privileges;                 /*  4  8 */

    int32_t m_Index;                              /*  4  4 */
    uint8_t m_Flags;                              /*  1  1 */
    /* vtable */                                  /*  4  8 */

    static std::vector<PropertyNodePtr> sm_EmptyChildNodes;

  private:
    void clearValue();

    int getAndRemoveIndexFromPropertyName(std::string& _propName);

    void childAdded(PropertyNodePtr _child);
    void childRemoved(PropertyNodePtr _child);
    void notifyListeners(void(PropertyListener::*_callback)(PropertyNodePtr, PropertyNodePtr), PropertyNodePtr _node);

  public:
    PropertyNode(const char* _name, int _index = 0);
    ~PropertyNode();

    static int getNodeCount();

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
    int size();

    /** Sets the value as string. */
    void setStringValue(const char* _value);
    /** Sets the value as string. */
    void setStringValue(const std::string& _value);
    /** Sets the value as integer. */
    void setIntegerValue(const int _value);
    /** Sets the value as uint32_t. */
    void setUnsignedIntegerValue(const uint32_t _value);
    /** Sets the value as boolean. */
    void setBooleanValue(const bool _value);
    /** Sets the value as floating. */
    void setFloatingValue(const double _value);

    template <typename T>
    void setValue(const T& value);

    /** Returns the string value.
     * Throws an exception if the value-types don't match. */
    std::string getStringValue() const;
    /** Returns the integer value.
     * Throws an exception if the value-types don't match. */
    int getIntegerValue() const;

    /** Returns the integer value.
     * Throws an exception if the value-types don't match. */
    uint32_t getUnsignedIntegerValue() const;

    /** Returns the boolean value.
     * Throws an exception if the value-types don't match. */
    bool getBoolValue() const;
    /** Returns the floating value.
     * Throws an exception if the value-types don't match. */
    double getFloatingValue() const;

    template <typename T>
    T getValue() const;

    /** Returns the value as string, regardless of it's value-type.*/
    std::string getAsString();

    /** Recursively adds a child-node. */
    PropertyNodePtr createProperty(const std::string& _propPath);

    /** Returns a child node by name.
     * Or NULL if not found.*/
    PropertyNodePtr getPropertyByName(const std::string& _name);

    /** Returns the type of the property. */
    aValueType getValueType();

    bool hasFlag(Flag _flag) const { return (m_Flags & _flag) == _flag; }
    bool searchForFlag(Flag _flag);
    void setFlag(Flag _flag, bool _value);

    void alias(PropertyNodePtr _target);
    bool IsAliased() { return m_AliasTarget != NULL; }

    // proxy support
    /** Links to the given proxy.
     * The incoming proxy will get cloned. */
    bool linkToProxy(const PropertyProxy<bool>& _proxy);
    /** @copydoc linkToProxy */
    bool linkToProxy(const PropertyProxy<int>& _proxy);
    /** @copydoc linkToProxy */
    bool linkToProxy(const PropertyProxy<std::string>& _proxy);
    /** @copydoc linkToProxy */
    bool linkToProxy(const PropertyProxy<double>& _proxy);
    /** @copydoc linkToProxy */
    bool linkToProxy(const PropertyProxy<uint32_t>& _proxy);
    /** Unlinks from a proxy */
    bool unlinkProxy(bool _recurse = false);
    bool IsLinkedToProxy() const { return m_Proxy.iValue != 0; }

    /** Adds a listener. */
    void addListener(PropertyListener* _listener);
    /** Removes a listener */
    void removeListener(PropertyListener* _listener);

    /** Returns the count of the nodes children. */
    int getChildCount() const {
      if (m_AliasTarget) {
        return m_AliasTarget->m_ChildNodes ? m_AliasTarget->m_ChildNodes->size() : 0;
      }
      return m_ChildNodes ? m_ChildNodes->size() : 0;
    }
    /** Returns a child node by index. */
    PropertyNodePtr getChild(const int _index) {
      if (m_AliasTarget) {
        return m_AliasTarget->m_ChildNodes ? m_AliasTarget->m_ChildNodes->at(_index) : PropertyNodePtr();
      }
      return m_ChildNodes ? m_ChildNodes->at(_index) : PropertyNodePtr();
    }

    const std::vector<PropertyNodePtr>& getChildNodes() const;

    /** Adds \a _childNode as a child to this node.
        If the node already has a parent, the node will be moved here. */
    void addChild(PropertyNodePtr _childNode);
    PropertyNodePtr removeChild(PropertyNodePtr _childNode);

    /** Returns the parent node of this node.
        @note Don't store this node anywhere */
    PropertyNode* getParentNode() { return m_ParentNode; }
    PropertyNode* getAliasTarget() { return m_AliasTarget; }

    /** Notifies all listeners that the value of this property has changed */
    void propertyChanged();

    NodePrivileges* getPrivileges() { return m_Privileges; }
    void setPrivileges(NodePrivileges* _value) { m_Privileges = _value; }

    /** Performs \a _callback for each child node (non-recursive) */
    void foreachChildOf(void(*_callback)(PropertyNode&)) {
      if (m_AliasTarget) {
        m_AliasTarget->foreachChildOf(_callback);
      } else if (NULL != m_ChildNodes) {
        for (PropertyList::iterator it = m_ChildNodes->begin(); it
            != m_ChildNodes->end(); ++it)
        {
          (*_callback)(**it);
        }
      }
    } // ForeachChildOf

    // needs to stay inside the header file, else the template specializations won't get generated ;-=
    /** @copydoc foreachChildOf */
    template<class Cls>
    void foreachChildOf(Cls& _objRef, void(Cls::*_callback)(PropertyNode&)) {
      if (m_AliasTarget) {
        m_AliasTarget->foreachChildOf(_objRef, _callback);
      } else if (NULL != m_ChildNodes) {
        for (PropertyList::iterator it = m_ChildNodes->begin(); it
            != m_ChildNodes->end(); ++it)
        {
          (_objRef.*_callback)(**it);
        }
      }
    }

    /// Return child property if it exists
    /// or create the child property with default value otherwise
    template <typename T>
    PropertyNodePtr getOrCreateChild(const std::string& childPath, const T& defaultValue) {
      auto child = getProperty(childPath);
      if (!child) {
        child = createProperty(childPath);
        child->setValue<T>(defaultValue);
      }
      // TODO(someday): verify that the property is of given type
      return child;
    }

    /// Return child property value if it exists
    /// or create the child property with default value and return its value.
    template <typename T>
    T getOrCreateChildValue(const std::string& childPath, const T& defaultValue) {
      return getOrCreateChild<T>(childPath, defaultValue)->template getValue<T>();
    }

  public:
    /** Writes the node to XML */
    bool saveAsXML(std::ostream& _os, const int _indent, const int _flagsMask);
    /** Saves the nodes children to XML */
    bool saveChildrenAsXML(std::ostream& _ofs, const int _indent, const int _flagsMask);

  }; // PropertyNode

  template <>
  inline void PropertyNode::setValue<std::string>(const std::string& value) { setStringValue(value); }
  template <>
  inline void PropertyNode::setValue<int>(const int& value) { setIntegerValue(value); }
  template <>
  inline void PropertyNode::setValue<unsigned int>(const unsigned int& value) { setUnsignedIntegerValue(value); }
  template <>
  inline void PropertyNode::setValue<bool>(const bool& value) { setBooleanValue(value); }
  template <>
  inline void PropertyNode::setValue<double>(const double& value) { setFloatingValue(value); }

  template <>
  inline std::string PropertyNode::getValue<std::string>() const { return getStringValue(); }
  template <>
  inline int PropertyNode::getValue<int>() const { return getIntegerValue(); }
  template <>
  inline unsigned int PropertyNode::getValue<unsigned int>() const { return getUnsignedIntegerValue(); }
  template <>
  inline bool PropertyNode::getValue<bool>() const { return getBoolValue(); }
  template <>
  inline double PropertyNode::getValue<double>() const { return getFloatingValue(); }

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
