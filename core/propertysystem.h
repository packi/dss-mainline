#ifndef NEUROPROPERTYSYSTEM_H
#define NEUROPROPERTYSYSTEM_H

#include <libxml/tree.h>
#include <libxml/xmlwriter.h>

#include <boost/mpl/if.hpp>
#include <boost/type_traits/is_class.hpp>

#include <vector>
#include <string>

namespace dss {

  class PropertyNode;

  typedef enum {
    vTypeNone = 0, vTypeInteger, vTypeString, vTypeBoolean
  } aValueType;

  const char* GetValueTypeAsString(aValueType _value);
  aValueType GetValueTypeFromString(const char* _str);

  typedef struct {
    aValueType valueType;
    union {
      char* pString;
      int Integer;
      bool Boolean;
    } actualValue;
    int PropertyLevel;
  } aPropertyValue;

  std::string GetBasePath(const std::string& _path);
  std::string GetProperty(const std::string& _path);
  std::string GetRoot(const std::string& _path);

  /**
   @author Patrick Staehlin <me@packi.ch>
   */
  class PropertySystem {
  private:
    PropertyNode* m_RootNode;
  public:
    PropertySystem();
    ~PropertySystem();

    bool LoadFromXML(const std::string& _fileName, PropertyNode* _rootNode = NULL);
    bool SaveToXML(const std::string& _fileName, PropertyNode* _rootNode = NULL) const;

    PropertyNode* GetProperty(const std::string& _propPath) const;

    PropertyNode* GetRootNode() const {
      return m_RootNode;
    }
    ;

    PropertyNode* CreateProperty(const std::string& _propPath);

    // fast access to property values
    int GetIntValue(const std::string& _propPath) const;
    bool GetBoolValue(const std::string& _propPath) const;
    std::string GetStringValue(const std::string& _propPath) const;

    bool SetIntValue(const std::string& _propPath, const int _value,
                     bool _mayCreate = true);
    bool SetBoolValue(const std::string& _propPath, const bool _value,
                      bool _mayCreate = true);
    bool SetStringValue(const std::string& _propPath, const std::string& _value,
                        bool _mayCreate = true);
  }; //  PropertySystem


  template<class T>
  class PropertyProxy {
  public:
    virtual ~PropertyProxy() {
    }
    ;

    static const T DefaultValue;
    virtual T GetValue() const = 0;
    virtual void SetValue(T _value) = 0;

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

    virtual T GetValue() const { return m_Reference; }

    virtual void SetValue(T _value) {
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

    virtual T GetValue() const {
      return *m_PointerToValue;
    }
    ;
    virtual void SetValue(T _value) {
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

    virtual T GetValue() const {
      if (m_GetterPtr != NULL) {
        return (*m_GetterPtr)();
      } else {
        return PropertyProxy<T>::DefaultValue;
      }
    } // GetValue

    virtual void SetValue(T _value) {
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
    typedef T (Cls::*aGetterType)() const;
    typedef void (Cls::*aSetterType)(typename boost::mpl::if_c<boost::is_class<T>::value, const T&, const T>::type);
    Cls& m_Obj;
    aGetterType m_GetterPtr;
    aSetterType m_SetterPtr;
  public:
    PropertyProxyMemberFunction(Cls& _obj, aGetterType _getter = 0,
                                aSetterType _setter = 0)
    :  m_Obj(_obj), m_GetterPtr(_getter), m_SetterPtr(_setter)
    { }

    virtual ~PropertyProxyMemberFunction() {
    }

    virtual T GetValue() const {
      if (m_GetterPtr != NULL) {
        return (m_Obj.*m_GetterPtr)();
      } else {
        return PropertyProxy<T>::DefaultValue;
      }
    } // GetValue

    virtual void SetValue(T _value) {
      if (m_SetterPtr != NULL) {
        (m_Obj.*m_SetterPtr)(_value);
      }
    } // SetValue

    virtual PropertyProxyMemberFunction* clone() const {
      return new PropertyProxyMemberFunction<Cls, T> (m_Obj, m_GetterPtr,
                                                      m_SetterPtr);
    }
  }; // PropertyProxyMemberFunction


  class PropertyListener {
  private:
    std::vector<const PropertyNode*> m_Properties;
  protected:
    friend class PropertyNode;
    virtual void PropertyChanged(const PropertyNode* _changedNode);

    void RegisterProperty(const PropertyNode* _node);
    void UnregisterProperty(const PropertyNode* _node);
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
    void AddChild(PropertyNode* _childNode);
    void ClearValue();

    int GetAndRemoveIndexFromPropertyName(std::string& _propName);

    void NotifyListeners(void(PropertyListener::*_callback)(const PropertyNode*));
  public:
    PropertyNode(PropertyNode* _parentNode, const char* _name, int _index = 0);
    ~PropertyNode();

    const std::string& GetName() const {
      return m_Name;
    }
    const std::string& GetDisplayName() const;

    PropertyNode* GetProperty(const std::string& _propPath);
    int Count(const std::string& _propertyName);

    void SetStringValue(const char* _value);
    void SetStringValue(const std::string& _value);
    void SetIntegerValue(const int _value);
    void SetBooleanValue(const bool _value);

    std::string GetStringValue();
    int GetIntegerValue();
    bool GetBoolValue();

    std::string GetAsString();

    PropertyNode* CreateProperty(const std::string& _propPath);
    void MoveTo(const std::string& _path);

    PropertyNode* GetPropertyByName(const std::string& _name);

    aValueType GetValueType();

    // proxy support
    bool LinkToProxy(const PropertyProxy<bool>& _proxy);
    bool LinkToProxy(const PropertyProxy<int>& _proxy);
    bool LinkToProxy(const PropertyProxy<std::string>& _proxy);
    bool UnlinkProxy();

    void AddListener(PropertyListener* _listener) const;
    void RemoveListener(PropertyListener* _listener) const;

    int GetChildCount() const { return m_ChildNodes.size(); }
    PropertyNode* GetChild(const int _index) { return m_ChildNodes.at(_index); }

    void ForeachChildOf(void(*_callback)(PropertyNode*)) {
      for (std::vector<PropertyNode*>::iterator it = m_ChildNodes.begin(); it
          != m_ChildNodes.end(); ++it) {
        (*_callback)(*it);
      }
    } // ForeachChildOf

    // needs to stay inside the header file, else the template specializations won't get generated ;-=
    template<class Cls>
    void ForeachChildOf(Cls& _objRef, void(Cls::*_callback)(PropertyNode*)) {
      for (std::vector<PropertyNode*>::iterator it = m_ChildNodes.begin(); it
          != m_ChildNodes.end(); ++it) {
        (_objRef.*_callback)(*it);
      }
    }
  public:
    bool SaveAsXML(xmlTextWriterPtr _writer);
    bool LoadFromNode(xmlNode* _pNode);
  }; // PropertyNode

} // namespace dss

#endif
