#ifndef SERIES_H_INCLUDED
#define SERIES_H_INCLUDED

#include "core/datetools.h"
#include "core/xmlwrapper.h"

#include <boost/shared_ptr.hpp>
#include <queue>

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
      m_Value = StrToDouble(_node.GetAttributes()["value"]);
      m_Min = StrToDouble(_node.GetAttributes()["min"]);
      m_Max = StrToDouble(_node.GetAttributes()["max"]);
      m_TimeStamp = DateTime::FromISO(_node.GetAttributes()["timestamp"]);
    } // ReadFromXMLNode

    const DateTime& GetTimeStamp() const { return m_TimeStamp; }
    void SetTimeStamp(const DateTime& _value) { m_TimeStamp = _value; }
    double GetMin() const { return m_Min; }
    double GetMax() const { return m_Max; }
    double GetValue() const { return m_Value; }
  }; // Value

  class AdderValue : public Value {
  public:
    AdderValue(double _value, const DateTime& _timeStamp)
    : Value(_value, _timeStamp)
    {}

    AdderValue(double _value)
    : Value(_value)
    {}

    virtual void ReadFromXMLNode(XMLNode& _node) {
      Value::ReadFromXMLNode(_node);
    }

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
      if(!m_Values.empty()) {
        Value& lastVal = m_Values.front();
        DateTime lastValStamp = lastVal.GetTimeStamp();
        int diff = _value.GetTimeStamp().Difference(lastValStamp);
        if(diff < m_Resolution) {
          lastVal.MergeWith(_value);
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
            lastValStamp = lastValStamp.AddSeconds(m_Resolution);
            m_Values.push_front(value_type(0, lastValStamp));
          }
          m_Values.push_front(_value);
          // if we've got an excess of values
          while(m_Values.size() > m_NumberOfValues) {
            value_type val = m_Values.back();
            m_Values.pop_back();
            if(m_NextSeries != NULL) {
              m_NextSeries->AddValue(val);
            }
          }
        }
      } else {
        // first value of series
        m_Values.push_front(_value);
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
  }; // Series

} // namespace dss

#endif // SERIES_H_INCLUDED
