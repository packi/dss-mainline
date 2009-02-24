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

namespace dss {

  //================================================== Metering

  Metering::Metering(DSS* _pDSS)
  : Subsystem(_pDSS, "Metering"),
    Thread("Metering"),
    m_MeterCheckIntervalSeconds(60)
  {
    m_Config.push_back(boost::shared_ptr<MeteringConfig>(new MeteringConfig("minutely", 1 * 60, 400)));
    m_Config.push_back(boost::shared_ptr<MeteringConfig>(new MeteringConfig("5minutely", 5 * 60, 400)));
    m_Config.push_back(boost::shared_ptr<MeteringConfig>(new MeteringConfig("hourly", 60 * 60, 400)));
    m_Config.push_back(boost::shared_ptr<MeteringConfig>(new MeteringConfig("daily", 24 * 60*60, 400)));
  } // Metering

  void Metering::Start() {
    Subsystem::Start();
    Run();
  } // Start

  void Metering::CheckModulators() {

    SeriesReader<AdderValue> reader;
    SeriesWriter<AdderValue> writer;
    std::vector<Modulator*>& modulators = DSS::GetInstance()->GetApartment().GetModulators();
    for(std::vector<Modulator*>::iterator ipModulator = modulators.begin(), e = modulators.end();
        ipModulator != e; ++ipModulator)
    {
      vector<boost::shared_ptr<Series<AdderValue> > > series;
      for(vector<boost::shared_ptr<MeteringConfig> >::iterator iConfig = m_Config.begin(), e = m_Config.end();
          iConfig != e; ++iConfig)
      {
        // Load series from file
        string fileName = m_MeteringStorageLocation + (*ipModulator)->GetDSID().ToString() + "_" + (*iConfig)->GetFilenameSuffix() + ".xml";
        Logger::GetInstance()->Log(string("Metering::CheckModulators: Trying to load series from '") + fileName + "'");
        if(FileExists(fileName)) {
          boost::shared_ptr<Series<AdderValue> > s = boost::shared_ptr<Series<AdderValue> >(reader.ReadFromXML(fileName));
          if(s.get() != NULL) {
            series.push_back(s);
          } else {
            Logger::GetInstance()->Log(string("Metering::CheckModulators: Failed to load series"));
            return; // TODO: another strategy would be moving the file out of our way and just create an empty one
          }
        } else {
          series.push_back(boost::shared_ptr<Series<AdderValue> >(new Series<AdderValue>((*iConfig)->GetResolution(), (*iConfig)->GetNumberOfValues())));
        }
      }
      // stitch up chain
      for(vector<boost::shared_ptr<Series<AdderValue> > >::iterator iSeries = series.begin(), e = series.end();
          iSeries != e; ++iSeries)
      {
        if(iSeries != series.begin()) {
          (*iSeries)->SetNextSeries(boost::prior(iSeries)->get());
        }
      }
      if(series.empty()) {
        Logger::GetInstance()->Log("Metering::CheckModulators: No series configured, check your config");
      } else {
        Logger::GetInstance()->Log("Metering::CheckModulators: Series loaded, updating");
        // Update series
        series[0]->AddValue((*ipModulator)->GetEnergyMeterValue(), DateTime());

        // Store series
        Logger::GetInstance()->Log("Metering::CheckModulators: Writing series back...");
        for(vector<boost::shared_ptr<MeteringConfig> >::iterator iConfig = m_Config.begin(), e = m_Config.end();
            iConfig != e; ++iConfig)
        {
          // Load series from file
          string fileName = m_MeteringStorageLocation + (*ipModulator)->GetDSID().ToString() + "_" + (*iConfig)->GetFilenameSuffix() + ".xml";
          Series<AdderValue>* s = series[distance(m_Config.begin(), iConfig)].get();
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
        SleepSeconds(40);
        continue;
      }

      Logger::GetInstance()->Log("Metering::Execute: Checking modulators");
      CheckModulators();
      Logger::GetInstance()->Log("Metering::Execute: Done checking modulators");
      SleepSeconds(m_MeterCheckIntervalSeconds);
    }
  } // Execute

} // namespace dss
