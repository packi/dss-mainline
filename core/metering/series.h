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
    Value(double _value)
    : m_Value(_value),
      m_Min(_value),
      m_Max(_value)
    { } // Value

    Value(double _value, const DateTime& _timeStamp)
    : m_Value(_value),
      m_Min(_value),
      m_Max(_value),
      m_TimeStamp(_timeStamp)
    { } // Value

    virtual ~Value() {};

    virtual void MergeWith(const Value& _other) {
      if(_other.GetMax() > m_Max) {
        m_Max = _other.GetMax();
      }
      if(_other.GetMin() < m_Min) {
        m_Min = _other.GetMin();
      }
    } // MergeWith

    virtual void ReadFromXMLNode(XMLNode& _node) {
      m_TimeStamp = DateTime(DateFromISOString(_node.GetAttributes()["timestamp"].c_str()));
      m_Min = StrToDouble(_node.GetChildByName("min").GetChildren()[0].GetContent());
      m_Max = StrToDouble(_node.GetChildByName("max").GetChildren()[0].GetContent());
      m_Value = StrToDouble(_node.GetChildByName("value").GetChildren()[0].GetContent());
    } // ReadFromXMLNode

    virtual void ReadFromXMLNode(Node* _node) {
      Element* elem = dynamic_cast<Element*>(_node);

      m_TimeStamp = DateTime(DateFromISOString(elem->getAttribute("timestamp").c_str()));
      m_Min = StrToDouble(elem->getChildElement("min")->firstChild()->getNodeValue());
      m_Max = StrToDouble(elem->getChildElement("max")->firstChild()->getNodeValue());
      m_Value = StrToDouble(elem->getChildElement("value")->firstChild()->getNodeValue());
    } // ReadFromXMLNode

    virtual void WriteToXMLNode(AutoPtr<Document>& _doc, AutoPtr<Element>& _elem) const {
      _elem->setAttribute("timestamp", (string)m_TimeStamp);

      AutoPtr<Element> elem = _doc->createElement("min");
      AutoPtr<Text> txt = _doc->createTextNode(DoubleToString(m_Min));
      elem->appendChild(txt);
      _elem->appendChild(elem);

      elem = _doc->createElement("max");
      txt = _doc->createTextNode(DoubleToString(m_Max));
      elem->appendChild(txt);
      _elem->appendChild(elem);

      elem = _doc->createElement("value");
      txt = _doc->createTextNode(DoubleToString(m_Value));
      elem->appendChild(txt);
      _elem->appendChild(elem);
    } // WriteToXMLNode

    const DateTime& GetTimeStamp() const { return m_TimeStamp; }
    void SetTimeStamp(const DateTime& _value) { m_TimeStamp = _value; }
    double GetMin() const { return m_Min; }
    double GetMax() const { return m_Max; }
    double GetValue() const { return m_Value; }
  }; // Value

  class CurrentValue : public Value {
  public:
    CurrentValue(double _value, const DateTime& _timeStamp)
    : Value(_value, _timeStamp)
    { }

    CurrentValue(double _value)
    : Value(_value)
    { }

    virtual void MergeWith(const Value& _other) {
      Value::MergeWith(_other);
      m_Value = _other.GetValue();
    }
  };

  class AdderValue : public Value {
  public:
    AdderValue(double _value, const DateTime& _timeStamp)
    : Value(_value, _timeStamp)
    {}

    AdderValue(double _value)
    : Value(_value)
    {}

    virtual void MergeWith(const Value& _other)
    {
      Value::MergeWith(_other);
      m_Value += _other.GetValue();
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

    void AddValue(const value_type& _value) {
      if(m_Resolution == 0) {
        throw runtime_error("Series::AddValue: m_Resolution is Zero. This will lead to an infinite loop");
      }
      if(!m_Values.empty()) {
        Value& lastVal = m_Values.front();
        DateTime lastValStamp = lastVal.GetTimeStamp();
        int diff = _value.GetTimeStamp().Difference(lastValStamp);
        if(diff < m_Resolution) {
          if(m_Values.size() > 1) {
            lastVal.MergeWith(_value);
          } else {
            // if we've got only one value the next incoming value has to be in the next interval
            DateTime prevStamp = m_Values.front().GetTimeStamp();
            value_type newVal = _value;
            newVal.SetTimeStamp(prevStamp.AddSeconds(m_Resolution));
            m_Values.push_front(newVal);
          }
        } else {
          // if we've got more than one value in the queue,
          // make sure we've got the right interval
          if(m_Values.size() > 1) {

            typename std::deque<value_type>::iterator iSecondValue = m_Values.begin();
            advance(iSecondValue, 1);
            DateTime prevStamp = iSecondValue->GetTimeStamp();
            lastValStamp = prevStamp.AddSeconds(m_Resolution);
            lastVal.SetTimeStamp(lastValStamp);
          }
          // add zero values where we don't have any data
          while(diff > m_Resolution) {
            diff -= m_Resolution;
            cout << "filling unknown value" << endl;
            lastValStamp = lastValStamp.AddSeconds(m_Resolution);
            m_Values.push_front(value_type(m_Values.front().GetValue(), lastValStamp));
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
        m_NextSeries->AddValue(_value);
      }	
    } // AddValue

    void AddValue(double _value, const DateTime& _timestamp) {
      AddValue(value_type(_value, _timestamp));
    } // AddValue

    std::deque<value_type>& GetValues() { return m_Values; }
    const std::deque<value_type>& GetValues() const { return m_Values; }

    void SetNextSeries(Series<T>* _value) { m_NextSeries = _value; }
    int GetNumberOfValues() const { return m_NumberOfValues; }
    int GetResolution() const { return m_Resolution; }

    const std::string& GetComment() const { return m_Comment; }
    void SetComment(const string& _value) { m_Comment = _value; }
    const dsid_t& GetFromDSID() const { return m_FromDSID; }
    void SetFromDSID(const dsid_t& _value) { m_FromDSID = _value; }
    const std::string GetUnit() const { return m_Unit; }
    void SetUnit(const string& _value) { m_Unit = _value; }
    bool Has(const string& _key) const { return m_Properties.Has(_key); }
    void Set(const string& _key, const string& _value)  { return m_Properties.Set(_key, _value); }
    const string& Get(const string& _key) const { return m_Properties.Get(_key); }
    const Properties& GetProperties() const { return m_Properties; }
    
  }; // Series

} // namespace dss

#endif // SERIES_H_INCLUDED
