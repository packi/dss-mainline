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

#include "metering.h"

#include <boost/filesystem.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>

#include "core/dss.h"
#include "core/logger.h"
#include "core/businterface.h"
#include "core/propertysystem.h"
#include "seriespersistence.h"
#include "core/foreach.h"
#include "core/model/modulator.h"
#include "core/model/apartment.h"
#include "core/model/modelmaintenance.h"
#include "core/security/security.h"

#ifdef LOG_TIMING
  #include <sstream>
#endif

namespace dss {

  //================================================== Metering

  Metering::Metering(DSS* _pDSS)
  : ThreadedSubsystem(_pDSS, "Metering"),
    m_pMeteringBusInterface(NULL)
  {
    m_ConfigConsumption.reset(new MeteringConfigChain(false, 1, "W"));
    m_ConfigConsumption->setComment("Consumption in W");
    m_ConfigConsumption->addConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig("consumption_seconds",        2, 400, false)));
    m_ConfigConsumption->addConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig("consumption_5minutely", 5 * 60, 400)));
    m_ConfigConsumption->addConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig("consumption_halfhourly",30 * 60, 400)));
    m_ConfigConsumption->addConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig("consumption_2hourly", 2 * 60*60, 400)));
    m_ConfigConsumption->addConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig("consumption_daily",  24 * 60*60, 400)));
    m_Config.push_back(m_ConfigConsumption);

    m_ConfigEnergy.reset(new MeteringConfigChain(true, 60, "Wh"));
    m_ConfigEnergy->setComment("Energymeter value");
    m_ConfigEnergy->addConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig("energy_minutely",  1 * 60, 400, false)));
    m_ConfigEnergy->addConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig("energy_5minutely", 5 * 60, 400)));
    m_ConfigEnergy->addConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig("energy_hourly",   60 * 60, 400)));
    m_ConfigEnergy->addConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig("energy_daily", 24 * 60*60, 400)));
    m_Config.push_back(m_ConfigEnergy);
  } // metering

  void Metering::initialize() {
    Subsystem::initialize();
    getDSS().getPropertySystem().setStringValue(getConfigPropertyBasePath() + "storageLocation", getDSS().getDataDirectory() + "metering/", true, false);
    m_MeteringStorageLocation = getDSS().getPropertySystem().getStringValue(getConfigPropertyBasePath() + "storageLocation");
    m_MeteringStorageLocation = addTrailingBackslash(m_MeteringStorageLocation);
    getDSS().getPropertySystem().setIntValue(getConfigPropertyBasePath() + "diffTimeMinSeconds", 20, true, false);
    getDSS().getPropertySystem().setIntValue(getConfigPropertyBasePath() + "cycleTimeSeconds", 300, true, false);

    if(getDSS().getPropertySystem().getBoolValue(
                                getConfigPropertyBasePath() + "enabled")) {
      if(!boost::filesystem::is_directory(m_MeteringStorageLocation)) {
        throw std::runtime_error("Metering directory " +
                                 m_MeteringStorageLocation +
                                 " does not exist!");
      }

      if(!rwAccess(m_MeteringStorageLocation)) {
        throw std::runtime_error("Metering directory " +
                                 m_MeteringStorageLocation +
                                 " is not readable/writable!");
      }
    }
  }
  void Metering::doStart() {
    log("Writing files to: " + m_MeteringStorageLocation);
    run();
  } // start

  void Metering::checkDSMeters() {
    m_pMeteringBusInterface->requestMeterData();
  } // checkDSMeters

  boost::shared_ptr<Series<CurrentValue> > Metering::createSeriesFromChain(
      boost::shared_ptr<MeteringConfigChain> _pChain,
      int _index,
      boost::shared_ptr<DSMeter> _pMeter) {
	kValueType stype = _pChain->isConsumption() ? kAverage : kMaximum;
    boost::shared_ptr<Series<CurrentValue> > result(
      new Series<CurrentValue>(_pChain->getResolution(_index),
                               _pChain->getNumberOfValues(_index),
                               stype));
    result->setUnit(_pChain->getUnit());
    result->setComment(_pChain->getComment());
    result->setFromDSID(_pMeter->getDSID());
    return result;
  } // createSeriesFromChain

  boost::shared_ptr<Series<CurrentValue> > Metering::getSeries(
                boost::shared_ptr<MeteringConfigChain> _pChain,
                int _index,
                boost::shared_ptr<DSMeter> _pMeter) {
    assert(_pChain != NULL);
    assert(_pChain->size() > _index);

    boost::shared_ptr<Series<CurrentValue> > result;
    boost::shared_ptr<MeteringConfig> config = _pChain->get(_index);
    if(config->isStored()) {

      std::string fileName = m_MeteringStorageLocation + _pMeter->getDSID().toString() + "_" + _pChain->getFilenameSuffix(_index) + ".xml";
      log("Metering::getSeries: Trying to load series from '" + fileName + "'");

      SeriesReader<CurrentValue> reader;
      result =
        boost::shared_ptr<Series<CurrentValue> >(reader.readFromXML(fileName));
      if(result == NULL) {
        log("Metering::getSeries: Failed to load series, creating a new one", lsDebug);
        result = createSeriesFromChain(_pChain, _index, _pMeter);
      }
    } else {
      result = createSeriesFromChain(_pChain, _index, _pMeter);
      m_ValuesMutex.lock();
      CachedSeriesMap::iterator ipSeries = m_CachedSeries.find(_pMeter);
      if(ipSeries != m_CachedSeries.end()) {
        if(_pChain->isConsumption()) {
          result->getValues() = ipSeries->second.first->getSeries()->getValues();
        } else {
          result->getValues() = ipSeries->second.second->getSeries()->getValues();
        }
      }
      m_ValuesMutex.unlock();
    }
    return result;
  } // getSeries

  void Metering::writeBackValuesOf(boost::shared_ptr<MeteringConfigChain> _pChain,
                                   std::deque<CurrentValue>& _values,
                                   DateTime& _lastStored,
                                   boost::shared_ptr<DSMeter> _pMeter) {
    typedef
    std::vector<boost::shared_ptr<Series<CurrentValue> > > SeriesVector;
    SeriesVector series;
    for(int iConfig = 1; iConfig < _pChain->size(); iConfig++) {
      series.push_back(getSeries(_pChain, iConfig, _pMeter));
    }

    log("Metering::processValue: Series loaded, updating");

    // stitch up chain
    for(SeriesVector::reverse_iterator iSeries = series.rbegin(),
        e = series.rend();
        iSeries != e;
        ++iSeries)  {
      if(iSeries != series.rbegin()) {
        (*iSeries)->setNextSeries(boost::prior(iSeries)->get());
      }
    }

    // Update series
    for(std::deque<CurrentValue>::iterator iValue = _values.begin(),
        e = _values.end();
        iValue != e; ++iValue) {
      if(iValue->getTimeStamp().after(_lastStored)) {
        series[0]->addValue(*iValue);
      }
    }

    // Store series
    log("Metering::processValue: Writing series back...");
    SeriesWriter<CurrentValue> writer;
    for(int iConfig = 1; iConfig < _pChain->size(); iConfig++) {
      // Write series to file
      std::string fileName = m_MeteringStorageLocation + _pMeter->getDSID().toString() + "_" + _pChain->getFilenameSuffix(iConfig) + ".xml";
      Series<CurrentValue>* s = series[iConfig-1].get();
      log("Metering::processValue: Trying to save series to '" + fileName + "'");
      writer.writeToXML(*s, fileName);
    }
  } // writeBackValues

  void Metering::writeBackValues() {
    // todo: bind properties to values
    DSS::getInstance()->getSecurity().loginAsSystemUser("Metering needs to get config values");
    while(!m_Terminated) {
      m_ValuesMutex.lock();
      unsigned int numSeriesInCycle = m_CachedSeries.size() * 2;
      m_ValuesMutex.unlock();

      int cycleTimeSeconds =
        getDSS().getPropertySystem().
          getIntValue(getConfigPropertyBasePath() + "cycleTimeSeconds");
      int diffTimeMinSeconds =
        getDSS().getPropertySystem().
          getIntValue(getConfigPropertyBasePath() + "diffTimeMinSeconds");

      if(numSeriesInCycle == 0) {
        sleepMS(400);
        continue;
      }

      int sleepTimeMS = std::max<int>(cycleTimeSeconds / numSeriesInCycle, diffTimeMinSeconds) * 1000;
      unsigned int currentCycle = 0;
      while((currentCycle < numSeriesInCycle) && !m_Terminated) {
        m_ValuesMutex.lock();
        if(currentCycle >= (m_CachedSeries.size() * 2)) {
          // the cycle map got smaller -> restart the cycle
          break;
        }
        CachedSeriesMap::iterator iCachedSeries = m_CachedSeries.begin();
        std::advance(iCachedSeries, currentCycle / 2);
        boost::shared_ptr<CachedSeries> series;
        if((currentCycle % 2) == 0) {
          series = iCachedSeries->second.first;
        } else {
          series = iCachedSeries->second.second;
        }
        // copy values so we can remove the lock
        bool touched = series->isTouched();
        DateTime lastStored;
        boost::shared_ptr<MeteringConfigChain> pChain;
        boost::shared_ptr<DSMeter> pMeter;
        std::deque<CurrentValue> values;
        if(touched) {
          lastStored = series->getLastStored();
          pChain = series->getChain();
          pMeter = iCachedSeries->first;
          values = series->getSeries()->getValues();
          series->markAsStored();
        }
        m_ValuesMutex.unlock();

        if(touched) {
          writeBackValuesOf(pChain, values, lastStored, pMeter);
        }

        currentCycle++;
        int cycleSleepTimeMS = sleepTimeMS;
        while((cycleSleepTimeMS > 0) && !m_Terminated) {
          const int kMaxSleeptimeMS = 400;
          sleepMS(std::min(kMaxSleeptimeMS, cycleSleepTimeMS));
          cycleSleepTimeMS -= kMaxSleeptimeMS;
        }
      }
    }
  } // writeBackValues

  void Metering::execute() {
    assert(m_pMeteringBusInterface != NULL);
    while(DSS::getInstance()->getModelMaintenance().isInitializing()) {
      sleepSeconds(1);
    }
    // spawn metering-data writing thread
    boost::thread th(boost::bind(&Metering::writeBackValues, this));
    // check dsMeters periodically
    while(!m_Terminated) {
      int sleepTimeMSec = 60000 * 1000;

      try {
        checkDSMeters();
      } catch (dss::BusApiError& apiError) {
        log("Metering::execute: Couldn't get metering data: " + std::string(apiError.what()));
      }
      for(unsigned int iConfig = 0; iConfig < m_Config.size(); iConfig++) {
        sleepTimeMSec = std::min(sleepTimeMSec, 1000 * m_Config[iConfig]->getCheckIntervalSeconds());
      }
      while(!m_Terminated && (sleepTimeMSec > 0)) {
        const int kMinSleepTimeMS = 100;
        sleepMS(std::min(sleepTimeMSec, kMinSleepTimeMS));
        sleepTimeMSec -= kMinSleepTimeMS;
      }
    }
    th.join();
  } // execute

  boost::shared_ptr<CachedSeries> Metering::getOrCreateCachedSeries(
        boost::shared_ptr<MeteringConfigChain> _pChain,
        boost::shared_ptr<DSMeter> _pMeter) {
    if(m_CachedSeries.find(_pMeter) == m_CachedSeries.end()) {
      boost::shared_ptr<Series<CurrentValue> > pSeriesConsumption(
        new Series<CurrentValue>(
          m_ConfigConsumption->getResolution(0),
          m_ConfigConsumption->getNumberOfValues(0),
          kAverage));
      pSeriesConsumption->setUnit(m_ConfigConsumption->getUnit());
      pSeriesConsumption->setComment(m_ConfigConsumption->getComment());
      pSeriesConsumption->setFromDSID(_pMeter->getDSID());
      boost::shared_ptr<CachedSeries> pCacheSeriesConsumption(
        new CachedSeries(pSeriesConsumption, m_ConfigConsumption));
      boost::shared_ptr<Series<CurrentValue> > pSeriesEnergy(
        new Series<CurrentValue>(
          m_ConfigEnergy->getResolution(0),
          m_ConfigEnergy->getNumberOfValues(0),
          kMaximum));
      pSeriesEnergy->setUnit(m_ConfigEnergy->getUnit());
      pSeriesEnergy->setComment(m_ConfigEnergy->getComment());
      pSeriesEnergy->setFromDSID(_pMeter->getDSID());
      boost::shared_ptr<CachedSeries> pCacheSeriesEnergy(
        new CachedSeries(pSeriesEnergy, m_ConfigEnergy));
      m_CachedSeries[_pMeter] = std::make_pair(pCacheSeriesConsumption, pCacheSeriesEnergy);
    }
    if(_pChain->isConsumption()) {
      return m_CachedSeries[_pMeter].first;
    } else {
      return m_CachedSeries[_pMeter].second;
    }
  } // getOrCreateCachedSeries

  void Metering::postConsumptionEvent(boost::shared_ptr<DSMeter> _meter,
                                      int _value,
                                      DateTime _sampledAt) {
    m_ValuesMutex.lock();
    boost::shared_ptr<CachedSeries> pSeries = getOrCreateCachedSeries(m_ConfigConsumption, _meter);
    pSeries->touch();
    pSeries->getSeries()->addValue(_value, _sampledAt);
    m_ValuesMutex.unlock();
  } // postConsumptionEvent

  void Metering::postEnergyEvent(boost::shared_ptr<DSMeter> _meter, int _value, DateTime _sampledAt) {
    m_ValuesMutex.lock();
    boost::shared_ptr<CachedSeries> pSeries = getOrCreateCachedSeries(m_ConfigEnergy, _meter);
    pSeries->touch();
    pSeries->getSeries()->addValue(_value, _sampledAt);
    m_ValuesMutex.unlock();
  } // postEnergyEvent

  //================================================== MeteringConfigChain

  void MeteringConfigChain::addConfig(boost::shared_ptr<MeteringConfig> _config) {
    m_Chain.push_back(_config);
  } // addConfig

} // namespace dss
