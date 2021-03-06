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

#include "src/subsystem.h"
#include "src/datetools.h"

#include <string>
#include <vector>
#include <deque>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/thread/mutex.hpp>

namespace dss {

  struct MeteringConfig;
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

  struct RRDLookup {
    RRDLookup(boost::shared_ptr<DSMeter> _meter, std::string& _file) :
      m_Meter(_meter), m_RrdFile(_file) {};
    boost::shared_ptr<DSMeter> m_Meter;
    std::string m_RrdFile;
  }; // RRDLookup

  class Metering : public ThreadedSubsystem {
  private:
    std::string m_MeteringStorageLocation;
    std::string m_RrdcachedPath;
    boost::shared_ptr<MeteringConfigChain> m_ConfigChain;
    std::vector<boost::shared_ptr<MeteringConfigChain> > m_Config;
    std::vector<RRDLookup> m_CachedSeries;
    boost::mutex m_cachedSeries_mutex;

    boost::mutex m_ValuesMutex;
    MeteringBusInterface* m_pMeteringBusInterface;
  private:
    virtual void initialize();
    virtual void execute();
    bool validateDBSeries(std::string& _fileName,
      boost::shared_ptr<MeteringConfigChain> _pChain,
      bool& _tunePowerMaxSetting);
    void tuneDBPowerSettings(std::string& _fileName);
    std::string getOrCreateCachedSeries(
      boost::shared_ptr<MeteringConfigChain> _pChain,
      boost::shared_ptr<DSMeter> _pMeter);
    int createDB(std::string& _filename, boost::shared_ptr<MeteringConfigChain> _pChain);
    void syncCachedDBValues();
  protected:
    // protected for testing
    bool checkDBReset(DateTime& _sampledAt, std::string& _rrdFileName);
    void checkAllDBReset(DateTime& _sampledAt);
    void checkDBConsistency();

    virtual void doStart();
  public:
    Metering(DSS* _pDSS);
    virtual ~Metering() {};

    typedef enum { etConsumption,  /**< Current power consumption. */
                   etEnergyDelta,  /**< Energy delta values. */
                   etEnergy,       /**< Energy counter. */
                 } SeriesTypes;

    const std::vector<boost::shared_ptr<MeteringConfigChain> >& getConfig() const { return m_Config; }
    const std::string& getStorageLocation() const { return m_MeteringStorageLocation; }
    void postMeteringEvent(boost::shared_ptr<DSMeter> _meter, unsigned int _valuePower, unsigned long long _valueEnergy, DateTime _sampledAt);
    void setMeteringBusInterface(MeteringBusInterface* _value) { m_pMeteringBusInterface = _value; }
    boost::shared_ptr<std::deque<Value> > getSeries(std::vector<boost::shared_ptr<DSMeter> > _meters,
                                                    int &_resolution,
                                                    SeriesTypes _type,
                                                    bool _energyInWh,
                                                    DateTime &_startTime,
                                                    DateTime &_endTime,
                                                    int &_valueCount);
    unsigned long getLastEnergyCounter(boost::shared_ptr<DSMeter> _meter);
  }; // Metering

  struct MeteringConfig {
    int m_Resolution;
    int m_NumberOfValues;
    MeteringConfig(int _resolution, int _numberOfValues)
    : m_Resolution(_resolution)
    , m_NumberOfValues(_numberOfValues)
    { } // ctor
  }; // MeteringConfig

  class MeteringConfigChain {

  private:
    int m_CheckIntervalSeconds;
    std::vector<MeteringConfig> m_Chain;

  public:
    MeteringConfigChain(int _checkIntervalSeconds);
    ~MeteringConfigChain();

    void addConfig(MeteringConfig _config);

    int size() const;

    int getResolution(int _index) const;
    int getNumberOfValues(int _index) const;

    int getCheckIntervalSeconds() const;
  }; // MeteringConfigChain

} // namespace dss

#endif /* METERING_H_ */
