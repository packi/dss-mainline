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
#include "core/propertysystem.h"
#include "seriespersistence.h"
#include "core/foreach.h"

namespace dss {

  //================================================== Metering

  Metering::Metering(DSS* _pDSS)
  : Subsystem(_pDSS, "Metering"),
    Thread("Metering")
  {
    GetDSS().GetPropertySystem().SetStringValue(GetConfigPropertyBasePath() + "storageLocation", GetDSS().GetDataDirectory() + "webroot/metering/", true);
    boost::shared_ptr<MeteringConfigChain> configConsumption(new MeteringConfigChain(false, 1, "mW"));
    configConsumption->SetComment("Consumption in mW");
    configConsumption->AddConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig("consumption_seconds",        2, 400)));
    configConsumption->AddConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig("consumption_10seconds",     10, 400)));
//    configConsumption->AddConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig("consumption_minutely",  1 * 60, 400)));
    configConsumption->AddConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig("consumption_5minutely", 5 * 60, 400)));
    configConsumption->AddConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig("consumption_halfhourly",30 * 60, 400)));
    configConsumption->AddConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig("consumption_2hourly", 2 * 60*60, 400)));
    configConsumption->AddConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig("consumption_daily",  24 * 60*60, 400)));
    m_Config.push_back(configConsumption);

    boost::shared_ptr<MeteringConfigChain> configEnergy(new MeteringConfigChain(true, 60, "Wh"));
    configEnergy->SetComment("Energymeter value");
    configEnergy->AddConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig("energy_minutely",  1 * 60, 400)));
    configEnergy->AddConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig("energy_5minutely", 5 * 60, 400)));
    configEnergy->AddConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig("energy_hourly",   60 * 60, 400)));
    configEnergy->AddConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig("energy_daily", 24 * 60*60, 400)));
    m_Config.push_back(configEnergy);
  } // Metering

  void Metering::DoStart() {
    m_MeteringStorageLocation = GetDSS().GetPropertySystem().GetStringValue(GetConfigPropertyBasePath() + "storageLocation");
    Log("Writing files to: " + m_MeteringStorageLocation);
    Run();
  } // Start

//#define LOG_TIMING

  void Metering::CheckModulators(boost::shared_ptr<MeteringConfigChain> _config) {
    SeriesReader<CurrentValue> reader;
    SeriesWriter<CurrentValue> writer;

    _config->Running();

#ifdef LOG_TIMING
    Timestamp checkingAll;
#endif
    std::vector<Modulator*>& modulators = DSS::GetInstance()->GetApartment().GetModulators();
    for(std::vector<Modulator*>::iterator ipModulator = modulators.begin(), e = modulators.end();
        ipModulator != e; ++ipModulator)
    {
#ifdef LOG_TIMING
      Timestamp checkingModulator;
      Timestamp startedLoading;
#endif
      vector<boost::shared_ptr<Series<CurrentValue> > > series;
      for(int iConfig = 0; iConfig < _config->Size(); iConfig++) {
        // Load series from file
        string fileName = m_MeteringStorageLocation + (*ipModulator)->GetDSID().ToString() + "_" + _config->GetFilenameSuffix(iConfig) + ".xml";
        Log(string("Metering::CheckModulators: Trying to load series from '") + fileName + "'");
        if(FileExists(fileName)) {
          Timestamp startedLoadingSingle;
          boost::shared_ptr<Series<CurrentValue> > s = boost::shared_ptr<Series<CurrentValue> >(reader.ReadFromXML(fileName));
#ifdef LOG_TIMING
          cout << "loading single: " <<  Timestamp().GetDifference(startedLoadingSingle) << endl;
#endif
          if(s.get() != NULL) {
            series.push_back(s);
          } else {
            Log(string("Metering::CheckModulators: Failed to load series"));
            return; // TODO: another strategy would be moving the file out of our way and just create an empty one
          }
        } else {
          boost::shared_ptr<Series<CurrentValue> > newSeries((new Series<CurrentValue>(_config->GetResolution(iConfig), _config->GetNumberOfValues(iConfig))));
          newSeries->SetUnit(_config->GetUnit());
          newSeries->SetComment(_config->GetComment());
          newSeries->SetFromDSID((*ipModulator)->GetDSID());
          series.push_back(newSeries);
        }
      }
#ifdef LOG_TIMING
      cout << "loading: " << Timestamp().GetDifference(startedLoading) << endl;
#endif

      // stitch up chain
      for(vector<boost::shared_ptr<Series<CurrentValue> > >::reverse_iterator iSeries = series.rbegin(), e = series.rend();
          iSeries != e; ++iSeries)
      {
        if(iSeries != series.rbegin()) {
          (*iSeries)->SetNextSeries(boost::prior(iSeries)->get());
        }
      }
      if(series.empty()) {
        Log("Metering::CheckModulators: No series configured, check your config");
      } else {
        Log("Metering::CheckModulators: Series loaded, updating");
        // Update series

        unsigned long value;
        DateTime timeRequested;
#ifdef LOG_TIMING
        Timestamp fetchingValue;
#endif
        if(_config->IsEnergy()) {
          value = (*ipModulator)->GetEnergyMeterValue();
        } else {
          value = (*ipModulator)->GetPowerConsumption();
        }
#ifdef LOG_TIMING
        cout << "fetching value: " << Timestamp().GetDifference(fetchingValue) << endl;
        Timestamp startedAddingValue;
#endif
        series[0]->AddValue(value, timeRequested);
#ifdef LOG_TIMING
        cout << "adding value: " << Timestamp().GetDifference(startedAddingValue) << endl;
#endif

#ifdef LOG_TIMING
        Timestamp startedWriting;
#endif
        // Store series
        Log("Metering::CheckModulators: Writing series back...");
        for(int iConfig = 0; iConfig < _config->Size(); iConfig++) {
#ifdef LOG_TIMING
          Timestamp startedWritingSingle;
#endif
          // Write series to file
          string fileName = m_MeteringStorageLocation + (*ipModulator)->GetDSID().ToString() + "_" + _config->GetFilenameSuffix(iConfig) + ".xml";
          Series<CurrentValue>* s = series[iConfig].get();
          Log(string("Metering::CheckModulators: Trying to save series to '") + fileName + "'");
          writer.WriteToXML(*s, fileName);
#ifdef LOG_TIMING
          cout << "writing single: " << Timestamp().GetDifference(startedWritingSingle) << endl;
#endif
        }
#ifdef LOG_TIMING
        cout << "writing: " << Timestamp().GetDifference(startedWriting) << endl;
#endif
      }
#ifdef LOG_TIMING
      cout << "checkingModulator: " << Timestamp().GetDifference(checkingModulator) << endl;
#endif
    }
#ifdef LOG_TIMING
    cout << "checking all: " << Timestamp().GetDifference(checkingAll) << endl;
#endif
  } // CheckModulators

  //#undef LOG_TIMING

  void Metering::Execute() {
    // check modulators periodically
    while(DSS::GetInstance()->GetApartment().IsInitializing()) {
      SleepSeconds(1);
    }
    while(!m_Terminated) {
      int sleepTimeSec = 60000;

      Log("Metering::Execute: Checking modulators");
      for(unsigned int iConfig = 0; iConfig < m_Config.size(); iConfig++) {
        if(m_Config[iConfig]->NeedsRun()) {
          CheckModulators(m_Config[iConfig]);
        }
        sleepTimeSec = std::min(sleepTimeSec, m_Config[iConfig]->GetCheckIntervalSeconds());
      }
      Log("Metering::Execute: Done checking modulators");
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
