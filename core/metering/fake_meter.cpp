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


#include "fake_meter.h"
#include "core/dss.h"
#include "core/propertysystem.h"
#include "seriespersistence.h"
#include "core/foreach.h"
#include "core/model/apartment.h"
#include "core/model/modulator.h"
#include "core/model/modelmaintenance.h"

#include <cmath>


namespace dss {
  FakeMeter::FakeMeter(DSS* _pDSS)
  : Subsystem(_pDSS, "FakeMeter"),
    Thread("FakeMeter")
  {

    m_Config.reset(new MeteringConfigChain(false, 1, "mW"));
    m_Config->setComment("Consumption in mW");
    m_Config->addConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig("consumption_seconds",        2, 400)));
    m_Config->addConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig("consumption_10seconds",     10, 400)));
    m_Config->addConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig("consumption_5minutely", 5 * 60, 400)));
    m_Config->addConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig("consumption_halfhourly",30 * 60, 400)));
    m_Config->addConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig("consumption_2hourly", 2 * 60*60, 400)));
    m_Config->addConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig("consumption_daily",  24 * 60*60, 400)));

    for(int iConfig = 0; iConfig < m_Config->size(); iConfig++) {
    	boost::shared_ptr<Series<CurrentValue> > newSeries((new Series<CurrentValue>(m_Config->getResolution(iConfig), m_Config->getNumberOfValues(iConfig))));
        newSeries->setUnit(m_Config->getUnit());
        newSeries->setComment(m_Config->getComment());
        newSeries->set("Customer-ID", "798UID88S9J");
        newSeries->set("Contract", "798UID88S9J-AX0T");
        newSeries->set("Name", "Jon Doe");
        m_Series.push_back(newSeries);
    }
    // stitch up chain
    for(std::vector<boost::shared_ptr<Series<CurrentValue> > >::reverse_iterator iSeries = m_Series.rbegin(), e = m_Series.rend();
      iSeries != e; ++iSeries)
    {
      if(iSeries != m_Series.rbegin()) {
        (*iSeries)->setNextSeries(boost::prior(iSeries)->get());
      }
    }
  }

  void FakeMeter::execute() {
    while(getDSS().getModelMaintenance().isInitializing()) {
      sleepSeconds(1);
    }
    SeriesWriter<CurrentValue> writer;

    std::string dev = getDSS().getPropertySystem().getStringValue(getConfigPropertyBasePath() + "device");
    bool addJitter = getDSS().getPropertySystem().getBoolValue(getConfigPropertyBasePath() + "addJitter");
    int holdTime = rand() % 15;
    unsigned long holdValue = 2 + (rand() % 5) * 10000;
	  srand(time(NULL));
    while(!m_Terminated) {
      sleepSeconds(1);
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

      DateTime now;
      unsigned long consumption = 0;
      foreach(DSMeter* dsMeter, getDSS().getApartment().getDSMeters()) {
        try {
      	  consumption += dsMeter->getPowerConsumption();
        } catch(std::runtime_error& err) {
          log("Could not poll dsMeter " + dsMeter->getDSID().toString() + ". Message: " + err.what());
        }
      }
      if(addJitter) {
        consumption += holdValue;
      }
      m_Series.front()->addValue(consumption, now);

      for(int iConfig = 0; iConfig < m_Config->size(); iConfig++) {
        // Write series to file
        std::string fileName = m_MeteringStorageLocation + "metering_" + m_Config->getFilenameSuffix(iConfig) + ".xml";
        Series<CurrentValue>* s = m_Series[iConfig].get();
        writer.writeToXML(*s, fileName);
      }
    }
  } // execute

  void FakeMeter::initialize() {
    Subsystem::initialize();

    getDSS().getPropertySystem().setStringValue(getConfigPropertyBasePath() + "device", "/dev/ttyS2", true, false);
    getDSS().getPropertySystem().setStringValue(getConfigPropertyBasePath() + "storageLocation", getDSS().getWebrootDirectory()+"metering/", true, false);
    getDSS().getPropertySystem().setBoolValue(getConfigPropertyBasePath() + "addJitter", false, true, false);
  }

  void FakeMeter::doStart() {
    m_MeteringStorageLocation = getDSS().getPropertySystem().getStringValue(getConfigPropertyBasePath() + "storageLocation");
    log("Writing files to: " + m_MeteringStorageLocation);
    run();
  } // doStart

}
