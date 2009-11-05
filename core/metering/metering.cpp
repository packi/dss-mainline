/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland
    Copyright (c) 2009 futureLAB AG, Winterthur, Switzerland

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

#include <boost/filesystem.hpp>

namespace dss {

  //================================================== Metering

  Metering::Metering(DSS* _pDSS)
  : Subsystem(_pDSS, "Metering"),
    Thread("Metering")
  {
    getDSS().getPropertySystem().setStringValue(getConfigPropertyBasePath() + "storageLocation", getDSS().getDataDirectory() + "webroot/metering/", true);
    boost::shared_ptr<MeteringConfigChain> configConsumption(new MeteringConfigChain(false, 1, "mW"));
    configConsumption->setComment("Consumption in mW");
    configConsumption->addConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig("consumption_seconds",        2, 400)));
    configConsumption->addConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig("consumption_10seconds",     10, 400)));
//    configConsumption->addConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig("consumption_minutely",  1 * 60, 400)));
    configConsumption->addConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig("consumption_5minutely", 5 * 60, 400)));
    configConsumption->addConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig("consumption_halfhourly",30 * 60, 400)));
    configConsumption->addConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig("consumption_2hourly", 2 * 60*60, 400)));
    configConsumption->addConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig("consumption_daily",  24 * 60*60, 400)));
    m_Config.push_back(configConsumption);

    boost::shared_ptr<MeteringConfigChain> configEnergy(new MeteringConfigChain(true, 60, "Wh"));
    configEnergy->setComment("Energymeter value");
    configEnergy->addConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig("energy_minutely",  1 * 60, 400)));
    configEnergy->addConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig("energy_5minutely", 5 * 60, 400)));
    configEnergy->addConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig("energy_hourly",   60 * 60, 400)));
    configEnergy->addConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig("energy_daily", 24 * 60*60, 400)));
    m_Config.push_back(configEnergy);
  } // metering

  void Metering::doStart() {
    m_MeteringStorageLocation = getDSS().getPropertySystem().getStringValue(getConfigPropertyBasePath() + "storageLocation");
    log("Writing files to: " + m_MeteringStorageLocation);
    run();
  } // start

//#define LOG_TIMING

  void Metering::checkModulators(boost::shared_ptr<MeteringConfigChain> _config) {
    SeriesReader<CurrentValue> reader;
    SeriesWriter<CurrentValue> writer;

    _config->running();

#ifdef LOG_TIMING
    Timestamp checkingAll;
#endif
    std::vector<Modulator*>& modulators = DSS::getInstance()->getApartment().getModulators();
    for(std::vector<Modulator*>::iterator ipModulator = modulators.begin(), e = modulators.end();
        ipModulator != e; ++ipModulator)
    {
      if(!(*ipModulator)->isPresent()) {
        continue;
      }
#ifdef LOG_TIMING
      Timestamp checkingModulator;
      Timestamp startedLoading;
#endif
      vector<boost::shared_ptr<Series<CurrentValue> > > series;
      for(int iConfig = 0; iConfig < _config->size(); iConfig++) {
        // Load series from file
        std::string fileName = m_MeteringStorageLocation + (*ipModulator)->getDSID().toString() + "_" + _config->getFilenameSuffix(iConfig) + ".xml";
        log("Metering::checkModulators: Trying to load series from '" + fileName + "'");
        if(boost::filesystem::exists(fileName)) {
          Timestamp startedLoadingSingle;
          boost::shared_ptr<Series<CurrentValue> > s = boost::shared_ptr<Series<CurrentValue> >(reader.readFromXML(fileName));
#ifdef LOG_TIMING
          cout << "loading single: " <<  Timestamp().getDifference(startedLoadingSingle) << endl;
#endif
          if(s.get() != NULL) {
            series.push_back(s);
          } else {
            log("Metering::checkModulators: Failed to load series");
            return; // TODO: another strategy would be moving the file out of our way and just create an empty one
          }
        } else {
          boost::shared_ptr<Series<CurrentValue> > newSeries((new Series<CurrentValue>(_config->getResolution(iConfig), _config->getNumberOfValues(iConfig))));
          newSeries->setUnit(_config->getUnit());
          newSeries->setComment(_config->getComment());
          newSeries->setFromDSID((*ipModulator)->getDSID());
          series.push_back(newSeries);
        }
      }
#ifdef LOG_TIMING
      cout << "loading: " << Timestamp().getDifference(startedLoading) << endl;
#endif

      // stitch up chain
      for(vector<boost::shared_ptr<Series<CurrentValue> > >::reverse_iterator iSeries = series.rbegin(), e = series.rend();
          iSeries != e; ++iSeries)
      {
        if(iSeries != series.rbegin()) {
          (*iSeries)->setNextSeries(boost::prior(iSeries)->get());
        }
      }
      if(series.empty()) {
        log("Metering::checkModulators: No series configured, check your config");
      } else {
        log("Metering::checkModulators: Series loaded, updating");
        // Update series

        unsigned long value;
        DateTime timeRequested;
#ifdef LOG_TIMING
        Timestamp fetchingValue;
#endif
        try {
          if(_config->isEnergy()) {
            value = (*ipModulator)->getEnergyMeterValue();
          } else {
            value = (*ipModulator)->getPowerConsumption();
          }
        } catch(std::runtime_error& err) {
          log("Could not poll modulator " + (*ipModulator)->getDSID().toString() + ". Message: " + err.what());
        }
#ifdef LOG_TIMING
        cout << "fetching value: " << Timestamp().getDifference(fetchingValue) << endl;
        Timestamp startedAddingValue;
#endif
        series[0]->addValue(value, timeRequested);
#ifdef LOG_TIMING
        cout << "adding value: " << Timestamp().getDifference(startedAddingValue) << endl;
#endif

#ifdef LOG_TIMING
        Timestamp startedWriting;
#endif
        // Store series
        log("Metering::checkModulators: Writing series back...");
        for(int iConfig = 0; iConfig < _config->size(); iConfig++) {
#ifdef LOG_TIMING
          Timestamp startedWritingSingle;
#endif
          // Write series to file
          std::string fileName = m_MeteringStorageLocation + (*ipModulator)->getDSID().toString() + "_" + _config->getFilenameSuffix(iConfig) + ".xml";
          Series<CurrentValue>* s = series[iConfig].get();
          log("Metering::checkModulators: Trying to save series to '" + fileName + "'");
          writer.writeToXML(*s, fileName);
#ifdef LOG_TIMING
          cout << "writing single: " << Timestamp().getDifference(startedWritingSingle) << endl;
#endif
        }
#ifdef LOG_TIMING
        cout << "writing: " << Timestamp().getDifference(startedWriting) << endl;
#endif
      }
#ifdef LOG_TIMING
      cout << "checkingModulator: " << Timestamp().getDifference(checkingModulator) << endl;
#endif
    }
#ifdef LOG_TIMING
    cout << "checking all: " << Timestamp().getDifference(checkingAll) << endl;
#endif
  } // checkModulators

  //#undef LOG_TIMING

  void Metering::execute() {
    // check modulators periodically
    while(DSS::getInstance()->getApartment().isInitializing()) {
      sleepSeconds(1);
    }
    while(!m_Terminated) {
      int sleepTimeSec = 60000;

      log("Metering::execute: Checking modulators");
      for(unsigned int iConfig = 0; iConfig < m_Config.size(); iConfig++) {
        if(m_Config[iConfig]->needsRun()) {
          checkModulators(m_Config[iConfig]);
        }
        sleepTimeSec = std::min(sleepTimeSec, m_Config[iConfig]->getCheckIntervalSeconds());
      }
      log("Metering::execute: Done checking modulators");
      sleepSeconds(sleepTimeSec);
    }
  } // execute


  //================================================== MeteringConfigChain

  void MeteringConfigChain::addConfig(boost::shared_ptr<MeteringConfig> _config) {
    m_Chain.push_back(_config);
  } // addConfig

  bool MeteringConfigChain::needsRun() const {
    return  DateTime().difference(m_LastRun) >= m_CheckIntervalSeconds;
  }

  void MeteringConfigChain::running() {
    m_LastRun = DateTime();
  }


} // namespace dss
