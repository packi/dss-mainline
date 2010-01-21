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

#include "metering.h"

#include "core/dss.h"
#include "core/logger.h"
#include "core/DS485Interface.h"
#include "core/propertysystem.h"
#include "seriespersistence.h"
#include "core/foreach.h"
#include "core/model/modulator.h"
#include "core/model/apartment.h"
#include "core/model/modelmaintenance.h"


#include <boost/filesystem.hpp>

#ifdef LOG_TIMING
  #include <sstream>
#endif

namespace dss {

  //================================================== Metering

  Metering::Metering(DSS* _pDSS)
  : Subsystem(_pDSS, "Metering"),
    Thread("Metering"),
    m_pMeteringBusInterface(NULL)
  {
    getDSS().getPropertySystem().setStringValue(getConfigPropertyBasePath() + "storageLocation", getDSS().getWebrootDirectory() + "metering/", true);
    m_ConfigConsumption.reset(new MeteringConfigChain(false, 1, "mW"));
    m_ConfigConsumption->setComment("Consumption in mW");
    m_ConfigConsumption->addConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig("consumption_seconds",        2, 400)));
    m_ConfigConsumption->addConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig("consumption_10seconds",     10, 400)));
    m_ConfigConsumption->addConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig("consumption_5minutely", 5 * 60, 400)));
    m_ConfigConsumption->addConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig("consumption_halfhourly",30 * 60, 400)));
    m_ConfigConsumption->addConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig("consumption_2hourly", 2 * 60*60, 400)));
    m_ConfigConsumption->addConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig("consumption_daily",  24 * 60*60, 400)));
    m_Config.push_back(m_ConfigConsumption.get());
    m_ConfigConsumption->running();

    m_ConfigEnergy.reset(new MeteringConfigChain(true, 60, "Wh"));
    m_ConfigEnergy->setComment("Energymeter value");
    m_ConfigEnergy->addConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig("energy_minutely",  1 * 60, 400)));
    m_ConfigEnergy->addConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig("energy_5minutely", 5 * 60, 400)));
    m_ConfigEnergy->addConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig("energy_hourly",   60 * 60, 400)));
    m_ConfigEnergy->addConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig("energy_daily", 24 * 60*60, 400)));
    m_Config.push_back(m_ConfigEnergy.get());
    m_ConfigEnergy->running();
  } // metering

  void Metering::doStart() {
    m_MeteringStorageLocation = getDSS().getPropertySystem().getStringValue(getConfigPropertyBasePath() + "storageLocation");
    log("Writing files to: " + m_MeteringStorageLocation);
    run();
  } // start

//#define LOG_TIMING

  void Metering::processValue(MeteringValue _value, boost::shared_ptr<MeteringConfigChain> _config) {
    SeriesReader<CurrentValue> reader;
    SeriesWriter<CurrentValue> writer;

    _config->running();
    #ifdef LOG_TIMING
    std::ostringstream logSStream;
    #endif

    #ifdef LOG_TIMING
    Timestamp checkingDSMeter;
    Timestamp startedLoading;
    #endif
    std::vector<boost::shared_ptr<Series<CurrentValue> > > series;
    for(int iConfig = 0; iConfig < _config->size(); iConfig++) {
      // Load series from file
      std::string fileName = m_MeteringStorageLocation + _value.getDSMeter().getDSID().toString() + "_" + _config->getFilenameSuffix(iConfig) + ".xml";
      log("Metering::processValue: Trying to load series from '" + fileName + "'");
      if(boost::filesystem::exists(fileName)) {
        Timestamp startedLoadingSingle;
        boost::shared_ptr<Series<CurrentValue> > s = boost::shared_ptr<Series<CurrentValue> >(reader.readFromXML(fileName));
        #ifdef LOG_TIMING
        logSStream << "loading single: " << Timestamp().getDifference(startedLoadingSingle);
        log(logSStream.str());
        logSStream.str("");
        #endif
        if(s.get() != NULL) {
          series.push_back(s);
        } else {
          log("Metering::processValue: Failed to load series");
          return; // TODO: another strategy would be moving the file out of our way and just create an empty one
        }
      } else {
        boost::shared_ptr<Series<CurrentValue> > newSeries((new Series<CurrentValue>(_config->getResolution(iConfig), _config->getNumberOfValues(iConfig))));
        newSeries->setUnit(_config->getUnit());
        newSeries->setComment(_config->getComment());
        newSeries->setFromDSID(_value.getDSMeter().getDSID());
        series.push_back(newSeries);
      }
    }
    #ifdef LOG_TIMING
    logSStream << "loading: " << Timestamp().getDifference(startedLoading);
    log(logMessage.str());
    logSStream.str("");
    #endif

    // stitch up chain
    for(std::vector<boost::shared_ptr<Series<CurrentValue> > >::reverse_iterator iSeries = series.rbegin(), e = series.rend();
    iSeries != e; ++iSeries)
    {
      if(iSeries != series.rbegin()) {
        (*iSeries)->setNextSeries(boost::prior(iSeries)->get());
      }
    }
    if(series.empty()) {
      log("Metering::processValue: No series configured, check your config");
    } else {
      log("Metering::processValue: Series loaded, updating");

      // Update series
      #ifdef LOG_TIMING
      Timestamp startedAddingValue;
      #endif
      series[0]->addValue(_value.getValue(), _value.getSampledAt());
      #ifdef LOG_TIMING
      logSStream << "adding value: " << Timestamp().getDifference(startedAddingValue);
      log(logSStream.str());
      logSStream.str("");
      #endif

      #ifdef LOG_TIMING
      Timestamp startedWriting;
      #endif
      // Store series
      log("Metering::processValue: Writing series back...");
      for(int iConfig = 0; iConfig < _config->size(); iConfig++) {
        #ifdef LOG_TIMING
        Timestamp startedWritingSingle;
        #endif
        // Write series to file
        std::string fileName = m_MeteringStorageLocation + _value.getDSMeter().getDSID().toString() + "_" + _config->getFilenameSuffix(iConfig) + ".xml";
        Series<CurrentValue>* s = series[iConfig].get();
        log("Metering::processValue: Trying to save series to '" + fileName + "'");
        writer.writeToXML(*s, fileName);
        #ifdef LOG_TIMING
        logSStream << "writing single: " << Timestamp().getDifference(startedWritingSingle) << endl;
        log(logSStream.str());
        logSStream.str("");
        #endif
      }
      #ifdef LOG_TIMING
      logSStream << "writing: " << Timestamp().getDifference(startedWriting) << endl;
      log(logSStream.str());
      logSStream.str("");
      #endif
    }
    #ifdef LOG_TIMING
    logSStream << "checkingDSMeter: " << Timestamp().getDifference(checkingDSMeter) << endl;
    log(logSStream.str());
    logSStream.str("");
    #endif
  } // processValue

  void Metering::checkDSMeters(MeteringConfigChain* _pConfig) {
    if(_pConfig->isConsumption()) {
      m_pMeteringBusInterface->requestPowerConsumption();
    } else {
      m_pMeteringBusInterface->requestEnergyMeterValue();
    }
    _pConfig->running();
  } // checkDSMeters

  //#undef LOG_TIMING

  void Metering::execute() {
    if(m_pMeteringBusInterface == NULL) {
      throw std::runtime_error("Missing bus interface");
    }
    // check dsMeters periodically
    while(DSS::getInstance()->getModelMaintenance().isInitializing()) {
      sleepSeconds(1);
    }
    while(!m_Terminated) {
      int sleepTimeMSec = 60000 * 1000;

      for(unsigned int iConfig = 0; iConfig < m_Config.size(); iConfig++) {
        if(m_Config[iConfig]->needsRun()) {
          checkDSMeters(m_Config[iConfig]);
        }
        sleepTimeMSec = std::min(sleepTimeMSec, 1000 * m_Config[iConfig]->getCheckIntervalSeconds());
      }
      DateTime startedSleeping;
      DateTime doneSleepingBy = DateTime().addSeconds(sleepTimeMSec / 1000);
      do {
        processValues(m_EnergyValues, m_ConfigEnergy);
        processValues(m_ConsumptionValues, m_ConfigConsumption);
        sleepMS(100); // wait roughly one token hold-time for new events
      } while(DateTime().before(doneSleepingBy));
    }
  } // execute

  void Metering::processValues(std::vector<MeteringValue>& _values, boost::shared_ptr<MeteringConfigChain> _config) {
    while(!_values.empty()) {
      m_ValuesMutex.lock();
      MeteringValue value = _values.front();
      _values.erase(_values.begin());
      m_ValuesMutex.unlock();
      processValue(value, _config);
    }
  } // processValues

  void Metering::postConsumptionEvent(dss::DSMeter& _meter, int _value, DateTime _sampledAt) {
    m_ValuesMutex.lock();
    m_ConsumptionValues.push_back(MeteringValue(&_meter, _value, _sampledAt));
    m_ValuesMutex.unlock();
  } // postConsumptionEvent

  void Metering::postEnergyEvent(dss::DSMeter& _meter, int _value, DateTime _sampledAt) {
    m_ValuesMutex.lock();
    m_EnergyValues.push_back(MeteringValue(&_meter, _value, _sampledAt));
    m_ValuesMutex.unlock();
  } // postEnergyEvent

  //================================================== MeteringConfigChain

  void MeteringConfigChain::addConfig(boost::shared_ptr<MeteringConfig> _config) {
    m_Chain.push_back(_config);
  } // addConfig

  bool MeteringConfigChain::needsRun() const {
    return DateTime().difference(m_LastRun) >= m_CheckIntervalSeconds;
  }

  void MeteringConfigChain::running() {
    m_LastRun = DateTime();
  }


} // namespace dss
