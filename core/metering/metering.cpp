/*
 * metering.cpp
 *
 *  Created on: Jan 29, 2009
 *      Author: patrick
 */

#include "metering.h"

#include "core/dss.h"
#include "core/logger.h"
#include "core/DS485Interface.h"
#include "seriespersistence.h"
#include "core/foreach.h"

namespace dss {

  //================================================== Metering

  Metering::Metering(DSS* _pDSS)
  : Subsystem(_pDSS, "Metering"),
    Thread("Metering")
  {
    m_MeteringStorageLocation = "data/webroot/metering/";
    boost::shared_ptr<MeteringConfigChain> configConsumption(new MeteringConfigChain(false, 1));
    configConsumption->AddConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig("consumption_seconds",        2, 400)));
    configConsumption->AddConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig("consumption_minutely",  1 * 60, 400)));
    configConsumption->AddConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig("consumption_5minutely", 5 * 60, 400)));
    configConsumption->AddConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig("consumption_hourly",   60 * 60, 400)));
    configConsumption->AddConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig("consumption_daily", 24 * 60*60, 400)));
    m_Config.push_back(configConsumption);

    boost::shared_ptr<MeteringConfigChain> configEnergy(new MeteringConfigChain(true, 60));
    configEnergy->AddConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig("energy_minutely",  1 * 60, 400)));
    configEnergy->AddConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig("energy_5minutely", 5 * 60, 400)));
    configEnergy->AddConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig("energy_hourly",   60 * 60, 400)));
    configEnergy->AddConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig("energy_daily", 24 * 60*60, 400)));
    m_Config.push_back(configEnergy);
  } // Metering

  void Metering::DoStart() {
    Run();
  } // Start

  void Metering::CheckModulators(boost::shared_ptr<MeteringConfigChain> _config) {
    SeriesReader<CurrentValue> reader;
    SeriesWriter<CurrentValue> writer;

    _config->Running();

    std::vector<Modulator*>& modulators = DSS::GetInstance()->GetApartment().GetModulators();
    for(std::vector<Modulator*>::iterator ipModulator = modulators.begin(), e = modulators.end();
        ipModulator != e; ++ipModulator)
    {
      vector<boost::shared_ptr<Series<CurrentValue> > > series;
      for(int iConfig = 0; iConfig < _config->Size(); iConfig++) {
        // Load series from file
        string fileName = m_MeteringStorageLocation + (*ipModulator)->GetDSID().ToString() + "_" + _config->GetFilenameSuffix(iConfig) + ".xml";
        Logger::GetInstance()->Log(string("Metering::CheckModulators: Trying to load series from '") + fileName + "'");
        if(FileExists(fileName)) {
          boost::shared_ptr<Series<CurrentValue> > s = boost::shared_ptr<Series<CurrentValue> >(reader.ReadFromXML(fileName));
          if(s.get() != NULL) {
            series.push_back(s);
          } else {
            Logger::GetInstance()->Log(string("Metering::CheckModulators: Failed to load series"));
            return; // TODO: another strategy would be moving the file out of our way and just create an empty one
          }
        } else {
          series.push_back(boost::shared_ptr<Series<CurrentValue> >(new Series<CurrentValue>(_config->GetResolution(iConfig), _config->GetNumberOfValues(iConfig))));
        }
      }
      // stitch up chain
      for(vector<boost::shared_ptr<Series<CurrentValue> > >::reverse_iterator iSeries = series.rbegin(), e = series.rend();
          iSeries != e; ++iSeries)
      {
        if(iSeries != series.rbegin()) {
          (*iSeries)->SetNextSeries(boost::prior(iSeries)->get());
        }
      }
      if(series.empty()) {
        Logger::GetInstance()->Log("Metering::CheckModulators: No series configured, check your config");
      } else {
        Logger::GetInstance()->Log("Metering::CheckModulators: Series loaded, updating");
        // Update series
        if(_config->IsEnergy()) {
          series[0]->AddValue((*ipModulator)->GetEnergyMeterValue(), DateTime());
        } else {
          series[0]->AddValue((*ipModulator)->GetPowerConsumption(), DateTime());
        }

        // Store series
        Logger::GetInstance()->Log("Metering::CheckModulators: Writing series back...");
        for(int iConfig = 0; iConfig < _config->Size(); iConfig++) {
          // Load series from file
          string fileName = m_MeteringStorageLocation + (*ipModulator)->GetDSID().ToString() + "_" + _config->GetFilenameSuffix(iConfig) + ".xml";
          Series<CurrentValue>* s = series[iConfig].get();
          Logger::GetInstance()->Log(string("Metering::CheckModulators: Trying to save series to '") + fileName + "'");
          writer.WriteToXML(*s, fileName);
        }
      }
    }
  } // CheckModulators

  void Metering::Execute() {
    // check modulators periodically
    while(!m_Terminated) {
      if(!DSS::GetInstance()->GetDS485Interface().IsReady()) {
        SleepSeconds(20);
        continue;
      }

      int sleepTimeSec = 60000;

      Logger::GetInstance()->Log("Metering::Execute: Checking modulators");
      for(unsigned int iConfig = 0; iConfig < m_Config.size(); iConfig++) {
        if(m_Config[iConfig]->NeedsRun()) {
          CheckModulators(m_Config[iConfig]);
        }
        sleepTimeSec = std::min(sleepTimeSec, m_Config[iConfig]->GetCheckIntervalSeconds());
      }
      Logger::GetInstance()->Log("Metering::Execute: Done checking modulators");
      SleepSeconds(sleepTimeSec);
    }
  } // Execute


  //================================================== MeteringConfigChain

  void MeteringConfigChain::AddConfig(boost::shared_ptr<MeteringConfig> _config) {
    m_Chain.push_back(_config);
  } // AddConfig

  bool MeteringConfigChain::NeedsRun() const {
    return  DateTime().Difference(m_LastRun) >= m_CheckIntervalSeconds;
  }

  void MeteringConfigChain::Running() {
    m_LastRun = DateTime();
  }


} // namespace dss
