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

#include <cmath>


namespace dss {
  FakeMeter::FakeMeter(DSS* _pDSS)
  : Subsystem(_pDSS, "FakeMeter"),
    Thread("FakeMeter")
  {
    GetDSS().GetPropertySystem().SetStringValue(GetConfigPropertyBasePath() + "device", "/dev/ttyS2");
    GetDSS().GetPropertySystem().SetStringValue(GetConfigPropertyBasePath() + "storageLocation", "data/webroot/metering/", true);
  }

  void FakeMeter::Execute() {
    SeriesWriter<CurrentValue> writer;

    string dev = GetDSS().GetPropertySystem().GetStringValue(GetConfigPropertyBasePath() + "device");
    m_Series.reset(new Series<CurrentValue>(60, 400));
    try {
      Log("Trying to open serial device '" + dev + "'", lsInfo);
      m_SerialCom.SetSpeed(SerialCom::sp9600);
      m_SerialCom.SetBlocking(true);
      m_SerialCom.Open(dev.c_str());
    } catch(runtime_error& e) {
      Log(string("Caught exception ") + e.what(), lsFatal);
      return;
    }
    while(!m_Terminated) {
      char c;
      // search for start-flag 'S'
      do {
        c = m_SerialCom.GetChar();
      } while(c != 'S');

      int val = 0;
      int iChar = 0;
      do {
        c = m_SerialCom.GetChar();
        if((c <= '9') && (c >='0')) {
          val += (c - '0') * (int)pow(10.0f, iChar++);
        } else if(c == 'E') {
        } else  {
          Log(string("invalid character '") + c + "'", lsError);
          break;
        }
      } while(!m_Terminated && c != 'E');
      Log("Found value " + IntToString(val));
      m_Series->AddValue(val, DateTime());
      writer.WriteToXML(*m_Series, m_MeteringStorageLocation + "metering.xml");
    }
  } // Execute

  void FakeMeter::DoStart() {
    m_MeteringStorageLocation = GetDSS().GetPropertySystem().GetStringValue(GetConfigPropertyBasePath() + "storageLocation");
    Log("Writing files to: " + m_MeteringStorageLocation);
    Run();
  } // DoStart

}
