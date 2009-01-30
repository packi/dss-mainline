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

  void Metering::CheckModulators() {

    SeriesReader<AdderValue> reader;
    std::vector<Modulator*>& modulators = DSS::GetInstance()->GetApartment().GetModulators();
    for(std::vector<Modulator*>::iterator ipModulator = modulators.begin(), e = modulators.end();
        ipModulator != e; ++ipModulator)
    {
      Series<AdderValue>* minutely = NULL;
      // Load series from file
      string fileName = m_MeteringStorageLocation + UnsignedLongIntToHexString((*ipModulator)->GetDSID()) + "_minutely.xml";
      if(FileExists(fileName)) {
        minutely = reader.ReadFromXML(fileName);
      } else {
        minutely = new Series<AdderValue>(60, 400);
      }
      // Update series
      minutely->AddValue((*ipModulator)->GetPowerConsumption(), DateTime());
      // Store series
    }
  } // CheckModulators

  void Metering::Execute() {
    // check modulators periodically
    while(!m_Terminated) {
      if(!DSS::GetInstance()->GetDS485Interface().IsReady()) {
        SleepSeconds(1);
        break;
      }

      Logger::GetInstance()->Log("Metering::Execute: Checking modulators");
      CheckModulators();
      Logger::GetInstance()->Log("Metering::Execute: Done checking modulators");
      SleepSeconds(m_MeterCheckIntervalSeconds);
    }
  } // Execute

} // namespace dss
