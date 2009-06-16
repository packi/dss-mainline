#ifndef SERIES_H_INCLUDED
#define SERIES_H_INCLUDED

#include "core/datetools.h"
#include "core/xmlwrapper.h"
#include "core/ds485types.h"

#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/Node.h>
#include <Poco/DOM/Text.h>
#include <Poco/DOM/NamedNodeMap.h>
#include <Poco/DOM/AutoPtr.h>

#include <boost/shared_ptr.hpp>
#include <queue>
#include <iostream>

using Poco::XML::Document;
using Poco::XML::Element;
using Poco::XML::Node;
using Poco::XML::Text;
using Poco::XML::AutoPtr;
using Poco::XML::NamedNodeMap;

namespace dss {

  class Value {
  protected:
    double m_Value;
  private:
    double m_Min;
    double m_Max;
    DateTime m_TimeStamp;
  public:
#ifdef WITH_GCOV
    Value();
#endif

    Value(double _value)
    : m_Value(_value),
      m_Min(_value),
      m_Max(_value)
    { } // ctor

    Value(double _value, const DateTime& _timeStamp)
    : m_Value(_value),
      m_Min(_value),
      m_Max(_value),
      m_TimeStamp(_timeStamp)
    { } // ctor

    virtual ~Value() {};

    virtual void mergeWith(const Value& _other) {
      if(_other.getMax() > m_Max) {
        m_Max = _other.getMax();
      }
      if(_other.getMin() < m_Min) {
        m_Min = _other.getMin();
      }
    } // mergeWith

    virtual void readFromXMLNode(XMLNode& _node) {
      m_TimeStamp = DateTime(dateFromISOString(_node.getAttributes()["timestamp"].c_str()));
      m_Min = strToDouble(_node.getChildByName("min").getChildren()[0].getContent());
      m_Max = strToDouble(_node.getChildByName("max").getChildren()[0].getContent());
      m_Value = strToDouble(_node.getChildByName("value").getChildren()[0].getContent());
    } // readFromXMLNode

    virtual void readFromXMLNode(Node* _node) {
      Element* elem = dynamic_cast<Element*>(_node);

      m_TimeStamp = DateTime(dateFromISOString(elem->getAttribute("timestamp").c_str()));
      m_Min = strToDouble(elem->getChildElement("min")->firstChild()->getNodeValue());
      m_Max = strToDouble(elem->getChildElement("max")->firstChild()->getNodeValue());
      m_Value = strToDouble(elem->getChildElement("value")->firstChild()->getNodeValue());
    } // readFromXMLNode

    virtual void writeToXMLNode(AutoPtr<Document>& _doc, AutoPtr<Element>& _elem) const {
      _elem->setAttribute("timestamp", (string)m_TimeStamp);

      AutoPtr<Element> elem = _doc->createElement("min");
      AutoPtr<Text> txt = _doc->createTextNode(doubleToString(m_Min));
      elem->appendChild(txt);
      _elem->appendChild(elem);

      elem = _doc->createElement("max");
      txt = _doc->createTextNode(doubleToString(m_Max));
      elem->appendChild(txt);
      _elem->appendChild(elem);

      elem = _doc->createElement("value");
      txt = _doc->createTextNode(doubleToString(m_Value));
      elem->appendChild(txt);
      _elem->appendChild(elem);
    } // writeToXMLNode

    const DateTime& getTimeStamp() const { return m_TimeStamp; }
    void setTimeStamp(const DateTime& _value) { m_TimeStamp = _value; }
    double getMin() const { return m_Min; }
    double getMax() const { return m_Max; }
    double getValue() const { return m_Value; }
  }; // Value

  class CurrentValue : public Value {
  public:
#ifdef WITH_GCOV
    CurrentValue();
#endif

    CurrentValue(double _value, const DateTime& _timeStamp)
    : Value(_value, _timeStamp)
    { }

    CurrentValue(double _value)
    : Value(_value)
    { }

    virtual void mergeWith(const Value& _other) {
      Value::mergeWith(_other);
      m_Value = _other.getValue();
    }
  };

  class AdderValue : public Value {
  public:
#ifdef WITH_GCOV
    AdderValue();
#endif

    AdderValue(double _value, const DateTime& _timeStamp)
    : Value(_value, _timeStamp)
    {}

    AdderValue(double _value)
    : Value(_value)
    {}

    virtual void mergeWith(const Value& _other)
    {
      Value::mergeWith(_other);
      m_Value += _other.getValue();
    }
  };

  template<class T>
  class Series {
  public:
    typedef T value_type;
  private:
    int m_Resolution;
    unsigned int m_NumberOfValues;
    Series<T>* m_NextSeries;
    std::deque<value_type> m_Values;
  private:
    std::string m_Comment;
    dsid_t m_FromDSID;
    std::string m_Unit;
    Properties m_Properties;
  public:
#ifdef WITH_GCOV
    Series();
#endif

    Series(const int _resolution, const unsigned int _numberOfValues)
    : m_Resolution(_resolution),
      m_NumberOfValues(_numberOfValues),
      m_NextSeries(NULL)
    { } // ctor

    Series(const int _resolution, const unsigned int _numberOfValues, Series<T>* _nextSeries)
    : m_Resolution(_resolution),
      m_NumberOfValues(_numberOfValues),
      m_NextSeries(_nextSeries)
    { } // ctor

    void addValue(const value_type& _value) {
      if(m_Resolution == 0) {
        throw runtime_error("Series::AddValue: m_Resolution is Zero. This will lead to an infinite loop");
      }
      if(!m_Values.empty()) {
        Value& lastVal = m_Values.front();
        DateTime lastValStamp = lastVal.getTimeStamp();
        int diff = _value.getTimeStamp().difference(lastValStamp);
        if(diff < m_Resolution) {
          if(m_Values.size() > 1) {
            lastVal.mergeWith(_value);
          } else {
            // if we've got only one value the next incoming value has to be in the next interval
            DateTime prevStamp = m_Values.front().getTimeStamp();
            value_type newVal = _value;
            newVal.setTimeStamp(prevStamp.addSeconds(m_Resolution));
            m_Values.push_front(newVal);
          }
        } else {
          // if we've got more than one value in the queue,
          // make sure we've got the right interval
          if(m_Values.size() > 1) {

            typename std::deque<value_type>::iterator iSecondValue = m_Values.begin();
            advance(iSecondValue, 1);
            DateTime prevStamp = iSecondValue->getTimeStamp();
            lastValStamp = prevStamp.addSeconds(m_Resolution);
            lastVal.setTimeStamp(lastValStamp);
          }
          // add zero values where we don't have any data
          while(diff > m_Resolution) {
            diff -= m_Resolution;
            cout << "filling unknown value" << endl;
            lastValStamp = lastValStamp.addSeconds(m_Resolution);
            m_Values.push_front(value_type(m_Values.front().getValue(), lastValStamp));
          }
          m_Values.push_front(_value);
          // if we've got an excess of values
          while(m_Values.size() > m_NumberOfValues) {
            m_Values.pop_back();
          }
        }
      } else {
        // first value of series
        m_Values.push_front(_value);
      }
      if(m_NextSeries != NULL) {
        m_NextSeries->addValue(_value);
      }
    } // addValue

    void addValue(double _value, const DateTime& _timestamp) {
      addValue(value_type(_value, _timestamp));
    } // addValue

    std::deque<value_type>& getValues() { return m_Values; }
    const std::deque<value_type>& getValues() const { return m_Values; }

    void setNextSeries(Series<T>* _value) { m_NextSeries = _value; }
    int getNumberOfValues() const { return m_NumberOfValues; }
    int getResolution() const { return m_Resolution; }

    const std::string& getComment() const { return m_Comment; }
    void setComment(const string& _value) { m_Comment = _value; }
    const dsid_t& getFromDSID() const { return m_FromDSID; }
    void setFromDSID(const dsid_t& _value) { m_FromDSID = _value; }
    const std::string getUnit() const { return m_Unit; }
    void setUnit(const string& _value) { m_Unit = _value; }
    bool has(const string& _key) const { return m_Properties.has(_key); }
    void set(const string& _key, const string& _value)  { return m_Properties.set(_key, _value); }
    const string& get(const string& _key) const { return m_Properties.get(_key); }
    const Properties& getProperties() const { return m_Properties; }

  }; // Series

} // namespace dss

#endif // SERIES_H_INCLUDED
