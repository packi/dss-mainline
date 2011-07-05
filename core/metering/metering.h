/*
    Copyright (c) 2009,2010 digitalSTROM.org, Zurich, Switzerland

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

#ifndef METERING_H_
#define METERING_H_

#include "core/thread.h"
#include "core/subsystem.h"
#include "core/datetools.h"
#include "core/mutex.h"
#include "core/metering/series.h"

#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/ptr_container/ptr_map.hpp>

namespace dss {

  class MeteringConfig;
  class MeteringConfigChain;
  class DSMeter;
  class MeteringValue;
  class MeteringBusInterface;

  class CachedSeries {
  public:
    CachedSeries(boost::shared_ptr<Series<CurrentValue> > _pSeries,
                 boost::shared_ptr<MeteringConfigChain> _pChain)
    : m_pSeries(_pSeries),
      m_pChain(_pChain),
      m_Touched(false)
    { }

    DateTime& getLastStored() {
      return m_LastStored;
    }

    void markAsStored() {
      m_LastStored = DateTime();
      m_Touched = false;
    }

    boost::shared_ptr<Series<CurrentValue> > getSeries() {
      return m_pSeries;
    }

    boost::shared_ptr<MeteringConfigChain> getChain() {
      return m_pChain;
    }

    void touch() {
      m_Touched = true;
    }

    bool isTouched() {
      return m_Touched;
    }
  private:
    boost::shared_ptr<Series<CurrentValue> > m_pSeries;
    boost::shared_ptr<MeteringConfigChain> m_pChain;
    DateTime m_LastStored;
    bool m_Touched;
  };

  class Metering : public ThreadedSubsystem {
  private:
    int m_MeterEnergyCheckIntervalSeconds;
    int m_MeterConsumptionCheckIntervalSeconds;
    std::string m_MeteringStorageLocation;
    boost::shared_ptr<MeteringConfigChain> m_ConfigEnergy;
    boost::shared_ptr<MeteringConfigChain> m_ConfigConsumption;
    std::vector<boost::shared_ptr<MeteringConfigChain> > m_Config;
    typedef
    std::map<boost::shared_ptr<DSMeter>,
      std::pair<
        boost::shared_ptr<CachedSeries>,
        boost::shared_ptr<CachedSeries> > > CachedSeriesMap;
    CachedSeriesMap m_CachedSeries;
    Mutex m_ValuesMutex;
    MeteringBusInterface* m_pMeteringBusInterface;
  private:
    void checkDSMeters();

    virtual void initialize();
    virtual void execute();
    void writeBackValuesOf(boost::shared_ptr<MeteringConfigChain> _pChain,
                           std::deque<CurrentValue>& _values,
                           DateTime& _lastStored,
                           boost::shared_ptr<DSMeter> _pMeter);
    void writeBackValues();
    boost::shared_ptr<CachedSeries> getOrCreateCachedSeries(
      boost::shared_ptr<MeteringConfigChain> _pChain,
      boost::shared_ptr<DSMeter> _pMeter);
    boost::shared_ptr<Series<CurrentValue> > createSeriesFromChain(
      boost::shared_ptr<MeteringConfigChain> _pChain,
      int _index,
      boost::shared_ptr<DSMeter> _pMeter);
  protected:
    virtual void doStart();
  public:
    Metering(DSS* _pDSS);
    virtual ~Metering() {};

    boost::shared_ptr<Series<CurrentValue> > getSeries(
      boost::shared_ptr<MeteringConfigChain> _pChain,
      int _index,
      boost::shared_ptr<DSMeter> _pMeter);

    const std::vector<boost::shared_ptr<MeteringConfigChain> > getConfig() const { return m_Config; }
    boost::shared_ptr<MeteringConfigChain> getEnergyConfigChain() { return m_ConfigEnergy; }
    const std::string& getStorageLocation() const { return m_MeteringStorageLocation; }
    void postConsumptionEvent(boost::shared_ptr<DSMeter> _meter, int _value, DateTime _sampledAt);
    void postEnergyEvent(boost::shared_ptr<DSMeter> _meter, int _value, DateTime _sampledAt);
    void setMeteringBusInterface(MeteringBusInterface* _value) { m_pMeteringBusInterface = _value; }
  }; // Metering

  class MeteringConfig {
  private:
    std::string m_FilenameSuffix;
    int m_Resolution;
    int m_NumberOfValues;
    bool m_Stored;
  public:
    MeteringConfig(const std::string& _filenameSuffix, int _resolution,
                   int _numberOfValues, bool _stored = true)
    : m_FilenameSuffix(_filenameSuffix),
      m_Resolution(_resolution),
      m_NumberOfValues(_numberOfValues),
      m_Stored(_stored)
    { } // ctor

    const std::string& getFilenameSuffix() const { return m_FilenameSuffix; }
    const int getResolution() const { return m_Resolution; }
    const int getNumberOfValues() const { return m_NumberOfValues; }
    const bool isStored() const { return m_Stored; }
  }; // MeteringConfig

  class MeteringConfigChain {
  private:
    bool m_IsEnergy;
    int m_CheckIntervalSeconds;
    std::string m_Unit;
    std::string m_Comment;
    std::vector<boost::shared_ptr<MeteringConfig> > m_Chain;
  public:
    MeteringConfigChain(bool _isEnergy, int _checkIntervalSeconds, const std::string& _unit)
    : m_IsEnergy(_isEnergy), m_CheckIntervalSeconds(_checkIntervalSeconds),
      m_Unit(_unit)
    { }

    void addConfig(boost::shared_ptr<MeteringConfig> _config);

    const int size() const { return m_Chain.size(); }
    boost::shared_ptr<MeteringConfig> get(int _index) { return m_Chain.at(_index); }

    const std::string& getFilenameSuffix(int _index) const { return m_Chain.at(_index)->getFilenameSuffix(); }
    const int getResolution(int _index) const { return m_Chain.at(_index)->getResolution(); }
    const int getNumberOfValues(int _index) const { return m_Chain.at(_index)->getNumberOfValues(); }

    bool isEnergy() const { return m_IsEnergy; }
    bool isConsumption() const { return !m_IsEnergy; }
    int getCheckIntervalSeconds() const { return m_CheckIntervalSeconds; }
    const std::string& getUnit() const { return m_Unit; }
    const std::string& getComment() const { return m_Comment; }
    void setComment(const std::string& _value) { m_Comment = _value; }
  }; // MeteringConfigChain

} // namespace dss

#endif /* METERING_H_ */
