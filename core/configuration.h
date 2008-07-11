#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#include "base.h"

#include <sstream>

namespace dss {
  class Config {
  private:
    // this field has to be mutable because accessing it by operator[] is non-constw
    mutable HashMapConstStringString m_OptionsByName; 
  public:
    bool HasOption(const string& _name) const;
    void ReadFromXML(const string& _fileName);
    
    void AssertHasOption(const string& _name) const;

    template <class t>
    t GetOptionAs(const string& _name) const;

    template <class t>
    t GetOptionAs(const string& _name, const t _defaultValue) const {
      if(HasOption(_name)) {
        return GetOptionAs<t>(_name);
      }
      return _defaultValue;
    }    
    
    template <class t>
    void SetOptionAs(const string& _name, const t& _value) {
      std::stringstream strstream;
      strstream << _value;
      m_OptionsByName[_name] = strstream.str();
    } // SetOptionAs<t>
    
    const HashMapConstStringString& GetOptions() { return m_OptionsByName; };
  }; // Config
  
  class NoSuchOptionException : public DSSException {
  private:
    const string m_OptionName;
  public:
    NoSuchOptionException(const string& _optionName) 
    : DSSException( string("Item not found:") ),
      m_OptionName(_optionName)
    {};
    
    ~NoSuchOptionException() throw() {};
    
    const string& GetOptionName() {
      return m_OptionName;
    } // GetOptionName
  }; // NoSuchOptionException
  

}

#endif /*CONFIGURATION_H_*/
