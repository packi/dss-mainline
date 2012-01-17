/*
    Copyright (c) 2009,2010,2011 digitalSTROM.org, Zurich, Switzerland

    Authors: Patrick Staehlin, futureLAB AG <pstaehlin@futurelab.ch>
             Christian Hitz, aizo AG <christian.hitz@aizo.com>

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

#ifndef METERING_H_
#define METERING_H_

#include "src/thread.h"
#include "src/subsystem.h"
#include "src/datetools.h"
#include "src/mutex.h"

#include <string>
#include <vector>
#include <deque>
#include <boost/shared_ptr.hpp>
#include <boost/ptr_container/ptr_map.hpp>

namespace dss {

  class MeteringConfig;
  class MeteringConfigChain;
  class DSMeter;
  class MeteringValue;
  class MeteringBusInterface;

  class Value {
  protected:
    double m_Value;
  private:
    DateTime m_TimeStamp;
  public:
#ifdef WITH_GCOV
    Value();
#endif

    Value(double _value)
    : m_Value(_value)
    { } // ctor

    Value(double _value, const DateTime& _timeStamp)
    : m_Value(_value),
      m_TimeStamp(_timeStamp)
    { } // ctor

    virtual ~Value() {};

    const DateTime& getTimeStamp() const { return m_TimeStamp; }
    void setTimeStamp(const DateTime& _value) { m_TimeStamp = _value; }
    double getValue() const { return m_Value; }
    void setValue(double _value) { m_Value = _value; }
  }; // Value

  class Metering : public ThreadedSubsystem {
  private:
    int m_MeterEnergyCheckIntervalSeconds;
    int m_MeterConsumptionCheckIntervalSeconds;
    std::string m_MeteringStorageLocation;
    std::string m_RrdcachedPath;
    boost::shared_ptr<MeteringConfigChain> m_ConfigChain;
    std::vector<boost::shared_ptr<MeteringConfigChain> > m_Config;
    typedef std::map<boost::shared_ptr<DSMeter>,
                     boost::shared_ptr<std::string> > CachedSeriesMap;
    CachedSeriesMap m_CachedSeries;
    Mutex m_ValuesMutex;
    MeteringBusInterface* m_pMeteringBusInterface;
  private:
    void checkDSMeters();

    virtual void initialize();
    virtual void execute();
    boost::shared_ptr<std::string> getOrCreateCachedSeries(
      boost::shared_ptr<MeteringConfigChain> _pChain,
      boost::shared_ptr<DSMeter> _pMeter);
  protected:
    virtual void doStart();
  public:
    Metering(DSS* _pDSS);
    virtual ~Metering() {};

    typedef enum { etConsumption,  /**< Current power consumption. */
                   etEnergyDelta,  /**< Energy delta values. */
                   etEnergy,       /**< Energy counter. */
                 } SeriesTypes;

    const std::vector<boost::shared_ptr<MeteringConfigChain> > getConfig() const { return m_Config; }
    const std::string& getStorageLocation() const { return m_MeteringStorageLocation; }
    void postMeteringEvent(boost::shared_ptr<DSMeter> _meter, int _valuePower, int _valueEnergy, DateTime _sampledAt);
    void setMeteringBusInterface(MeteringBusInterface* _value) { m_pMeteringBusInterface = _value; }
    boost::shared_ptr<std::deque<Value> > getSeries(boost::shared_ptr<DSMeter> _meter,
                                                    int &_resolution,
                                                    SeriesTypes _type,
                                                    bool _energyInWh,
                                                    DateTime &_startTime,
                                                    DateTime &_endTime,
                                                    int &_valueCount);
    unsigned long getLastEnergyCounter(boost::shared_ptr<DSMeter> _meter);
  }; // Metering

  class MeteringConfig {
  private:
    int m_Resolution;
    int m_NumberOfValues;
  public:
    MeteringConfig(int _resolution,
                   int _numberOfValues)
    : m_Resolution(_resolution)
    , m_NumberOfValues(_numberOfValues)
    { } // ctor

    const int getResolution() const { return m_Resolution; }
    const int getNumberOfValues() const { return m_NumberOfValues; }
  }; // MeteringConfig

  class MeteringConfigChain {
  private:
    int m_CheckIntervalSeconds;
    std::vector<boost::shared_ptr<MeteringConfig> > m_Chain;
  public:
    MeteringConfigChain(int _checkIntervalSeconds)
    : m_CheckIntervalSeconds(_checkIntervalSeconds)
    { }

    void addConfig(boost::shared_ptr<MeteringConfig> _config);

    const int size() const { return m_Chain.size(); }
    boost::shared_ptr<MeteringConfig> get(int _index) { return m_Chain.at(_index); }

    const int getResolution(int _index) const { return m_Chain.at(_index)->getResolution(); }
    const int getNumberOfValues(int _index) const { return m_Chain.at(_index)->getNumberOfValues(); }

    int getCheckIntervalSeconds() const { return m_CheckIntervalSeconds; }
  }; // MeteringConfigChain

} // namespace dss

#endif /* METERING_H_ */
