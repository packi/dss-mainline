/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland

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

#ifndef SERIES_H_INCLUDED
#define SERIES_H_INCLUDED

#include "core/datetools.h"
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

    virtual void readFromXMLNode(Node* _node) {
      Element* elem = dynamic_cast<Element*>(_node);

      m_TimeStamp = DateTime(dateFromISOString(elem->getAttribute("timestamp").c_str()));
      m_Min = strToDouble(elem->getChildElement("min")->firstChild()->getNodeValue());
      m_Max = strToDouble(elem->getChildElement("max")->firstChild()->getNodeValue());
      m_Value = strToDouble(elem->getChildElement("value")->firstChild()->getNodeValue());
    } // readFromXMLNode

    virtual void writeToXMLNode(AutoPtr<Document>& _doc, AutoPtr<Element>& _elem) const {
      _elem->setAttribute("timestamp", (std::string)m_TimeStamp);

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
    typedef std::deque<value_type> QueueType;
  private:
    int m_Resolution;
    unsigned int m_NumberOfValues;
    Series<T>* m_NextSeries;
    QueueType m_Values;
  private:
    std::string m_Comment;
    dss_dsid_t m_FromDSID;
    std::string m_Unit;
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
        throw std::runtime_error("Series::AddValue: m_Resolution is Zero. This will lead to an infinite loop");
      }
      DateTime bucketTimeStamp(static_cast<time_t>(_value.getTimeStamp().secondsSinceEpoch() -
        (_value.getTimeStamp().secondsSinceEpoch() % m_Resolution)));
      value_type newValue = _value;
      newValue.setTimeStamp(bucketTimeStamp);
      if(!m_Values.empty()) {
        Value& lastVal = m_Values.front();
        DateTime lastValStamp = lastVal.getTimeStamp();
        if(lastValStamp == bucketTimeStamp) {
          lastVal.mergeWith(_value);
        } else if (bucketTimeStamp.after(lastValStamp)) {
          m_Values.push_front(newValue);
        } else {
          typename QueueType::iterator iValue = m_Values.begin(), e = m_Values.end();
          while(iValue != e) {
            DateTime currentStamp = iValue->getTimeStamp();
            if(currentStamp <= bucketTimeStamp) {
              break;
            }
            ++iValue;
          }
          if (iValue == e) {
            m_Values.push_back(newValue);
          } else if(bucketTimeStamp == iValue->getTimeStamp()) {
            iValue->mergeWith(_value);
          } else {
            m_Values.insert(iValue, newValue);
          }
        }
      } else {
        // insert at front
        m_Values.push_front(newValue);
      }
      DateTime oldestBucket = m_Values.front().getTimeStamp();
      oldestBucket = oldestBucket.addSeconds( -(m_Resolution*(m_NumberOfValues-1)) );
      while((m_Values.size() > m_NumberOfValues) ||
             ((m_Values.size() > 1) && (m_Values.back().getTimeStamp() < oldestBucket))) {
        m_Values.pop_back();
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
    void setComment(const std::string& _value) { m_Comment = _value; }
    const dss_dsid_t& getFromDSID() const { return m_FromDSID; }
    void setFromDSID(const dss_dsid_t& _value) { m_FromDSID = _value; }
    const std::string getUnit() const { return m_Unit; }
    void setUnit(const std::string& _value) { m_Unit = _value; }

    boost::shared_ptr<std::deque<value_type> > getExpandedValues() {
      boost::shared_ptr<std::deque<value_type> > result(new std::deque<value_type>);
      std::deque<value_type>* expandedQueue = result.get();

      typename QueueType::iterator iValue = m_Values.begin(), e = m_Values.end();

      DateTime iCurrentTimeStamp;
      iCurrentTimeStamp = iCurrentTimeStamp.addSeconds(-(iCurrentTimeStamp.secondsSinceEpoch() % m_Resolution));
      if(iCurrentTimeStamp < m_Values.front().getTimeStamp()) {
        iCurrentTimeStamp = m_Values.front().getTimeStamp();
      }
      while( (iValue != e) && (expandedQueue->size() < m_NumberOfValues) ) {
        value_type val = *iValue;
        val.setTimeStamp(iCurrentTimeStamp);
        expandedQueue->push_back(val);
        if(iValue->getTimeStamp() == iCurrentTimeStamp) {
          iValue++;
        }
        iCurrentTimeStamp = iCurrentTimeStamp.addSeconds(-m_Resolution);
      }
      return result;
    }

  }; // Series

} // namespace dss

#endif // SERIES_H_INCLUDED
