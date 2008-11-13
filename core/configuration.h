#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#include "base.h"

#include <sstream>

namespace dss {
  /** Configuration helper, holds name-value pairs */
  class Config {
  private:
    // this field has to be mutable because accessing it by operator[] is non-const
    mutable HashMapConstStringString m_OptionsByName;

    void AssertHasOption(const string& _name) const;
  public:
    /** Returns true if the option named _name exists */
    bool HasOption(const string& _name) const;
    /** Loads the config data from an xml-file. Note that this
      * does not clear the options.
      */
    void ReadFromXML(const string& _fileName);

    /** Returns the option converted to the type of t */
    template <class t>
    t GetOptionAs(const string& _name) const;

    /** Returns the option converted to the type of t if present. Else
      * _defaultValue will be returned.
      */
    template <class t>
    t GetOptionAs(const string& _name, const t _defaultValue) const {
      if(HasOption(_name)) {
        return GetOptionAs<t>(_name);
      }
      return _defaultValue;
    }

    /** Sets the option named _name to _value */
    template <class t>
    void SetOptionAs(const string& _name, const t& _value) {
      std::stringstream strstream;
      strstream << _value;
      m_OptionsByName[_name] = strstream.str();
    } // SetOptionAs<t>

    /** Returns all options in a const hash */
    const HashMapConstStringString& GetOptions() { return m_OptionsByName; }
  }; // Config

  class NoSuchOptionException : public DSSException {
  private:
    const string m_OptionName;
  public:
    NoSuchOptionException(const string& _optionName)
    : DSSException( string("Item not found:") ),
      m_OptionName(_optionName)
    {}

    ~NoSuchOptionException() throw() {}

    const string& GetOptionName() {
      return m_OptionName;
    } // GetOptionName
  }; // NoSuchOptionException


}

#endif /*CONFIGURATION_H_*/
