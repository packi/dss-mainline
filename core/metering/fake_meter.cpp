/*
 * fake_meter.cpp
 *
 *  Created on: Apr 8, 2009
 *      Author: patrick
 */

#include "fake_meter.h"
#include "core/dss.h"
#include "core/propertysystem.h"
#include "seriespersistence.h"
#include "core/model.h"
#include "core/foreach.h"

#include <cmath>


namespace dss {
  FakeMeter::FakeMeter(DSS* _pDSS)
  : Subsystem(_pDSS, "FakeMeter"),
    Thread("FakeMeter")
  {

    m_Config.reset(new MeteringConfigChain(false, 1, "mW"));
    m_Config->SetComment("Consumption in mW");
    m_Config->AddConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig("consumption_seconds",        2, 400)));
    m_Config->AddConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig("consumption_10seconds",     10, 400)));
    m_Config->AddConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig("consumption_5minutely", 5 * 60, 400)));
    m_Config->AddConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig("consumption_halfhourly",30 * 60, 400)));
    m_Config->AddConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig("consumption_2hourly", 2 * 60*60, 400)));
    m_Config->AddConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig("consumption_daily",  24 * 60*60, 400)));

    for(int iConfig = 0; iConfig < m_Config->Size(); iConfig++) {
    	boost::shared_ptr<Series<CurrentValue> > newSeries((new Series<CurrentValue>(m_Config->GetResolution(iConfig), m_Config->GetNumberOfValues(iConfig))));
        newSeries->SetUnit(m_Config->GetUnit());
        newSeries->SetComment(m_Config->GetComment());
        newSeries->Set("Customer-ID", "798UID88S9J");
        newSeries->Set("Contract", "798UID88S9J-AX0T");
        newSeries->Set("Name", "Jon Doe");
        m_Series.push_back(newSeries);
    }
    // stitch up chain
    for(vector<boost::shared_ptr<Series<CurrentValue> > >::reverse_iterator iSeries = m_Series.rbegin(), e = m_Series.rend();
      iSeries != e; ++iSeries)
    {
      if(iSeries != m_Series.rbegin()) {
        (*iSeries)->SetNextSeries(boost::prior(iSeries)->get());
      }
    }
  }

  void FakeMeter::Execute() {
    while(GetDSS().GetApartment().IsInitializing()) {
      SleepSeconds(1);
    }
    SeriesWriter<CurrentValue> writer;

    string dev = GetDSS().GetPropertySystem().GetStringValue(GetConfigPropertyBasePath() + "device");
    bool addJitter = GetDSS().GetPropertySystem().GetBoolValue(GetConfigPropertyBasePath() + "addJitter");
    int holdTime = rand() % 15;
    unsigned long holdValue = 2 + (rand() % 5) * 10000;
	  srand(time(NULL));
    while(!m_Terminated) {
      SleepSeconds(1);
      if(holdTime <= 0) {
      	holdTime = 2 + (rand() % 5);
      	if((rand() % 10) >= 5) {
	      	holdValue += (rand() % 2) * 10000;
	    } else {
	    	holdValue -= (rand() % 2) * 10000;
	    }
	    if(holdValue > 100000) {
	    	holdValue = 100000;
	    }
      } else {
      	holdTime--;
      }

      DateTime now = DateTime();
      unsigned long consumption = 0;
      foreach(Modulator* modulator, GetDSS().GetApartment().GetModulators()) {
      	consumption += modulator->GetPowerConsumption();
      }
      if(addJitter) {
        consumption += holdValue;
      }
      m_Series.front()->AddValue(consumption, now);

      for(int iConfig = 0; iConfig < m_Config->Size(); iConfig++) {
        // Write series to file
        string fileName = m_MeteringStorageLocation + "metering_" + m_Config->GetFilenameSuffix(iConfig) + ".xml";
        Series<CurrentValue>* s = m_Series[iConfig].get();
        writer.WriteToXML(*s, fileName);
      }
    }
  } // Execute

  void FakeMeter::Initialize() {
    Subsystem::Initialize();

    GetDSS().GetPropertySystem().SetStringValue(GetConfigPropertyBasePath() + "device", "/dev/ttyS2", true, false);
    GetDSS().GetPropertySystem().SetStringValue(GetConfigPropertyBasePath() + "storageLocation", GetDSS().GetDataDirectory()+"webroot/metering/", true, false);
    GetDSS().GetPropertySystem().SetBoolValue(GetConfigPropertyBasePath() + "addJitter", false, true, false);
  }

  void FakeMeter::DoStart() {
    m_MeteringStorageLocation = GetDSS().GetPropertySystem().GetStringValue(GetConfigPropertyBasePath() + "storageLocation");
    Log("Writing files to: " + m_MeteringStorageLocation);
    Run();
  } // DoStart

}
