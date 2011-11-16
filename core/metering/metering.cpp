/*
    Copyright (c) 2009,2010,2011 digitalSTROM.org, Zurich, Switzerland

    Authors: Patrick Staehlin, futureLAB AG <pstaehlin@futurelab.ch>
             Christian Hitz, aizo AG <christian.hitz@aizo.com>

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

#include "core/dss.h"
#include "core/logger.h"
#include "core/businterface.h"
#include "core/propertysystem.h"
#include "core/foreach.h"
#include "core/model/modulator.h"
#include "core/model/apartment.h"
#include "core/model/modelmaintenance.h"
#include "core/security/security.h"

#include <algorithm>
#include <functional>
#include <iterator>

#include <rrd.h>

#ifdef LOG_TIMING
  #include <sstream>
#endif

namespace dss {

  //================================================== Metering

  Metering::Metering(DSS* _pDSS)
    : ThreadedSubsystem(_pDSS, "Metering")
    , m_pMeteringBusInterface(NULL) {
    m_ConfigConsumption.reset(new MeteringConfigChain(1));
    m_ConfigConsumption->addConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig(           1, 400)));
    m_ConfigConsumption->addConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig(          60, 400)));
    m_ConfigConsumption->addConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig(     15 * 60, 400)));
    m_ConfigConsumption->addConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig(     60 * 60, 400)));
    m_ConfigConsumption->addConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig( 3 * 60 * 60, 400)));
    m_ConfigConsumption->addConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig(24 * 60 * 60, 400)));
    m_Config.push_back(m_ConfigConsumption);
  } // metering

  void Metering::initialize() {
    Subsystem::initialize();
    getDSS().getPropertySystem().setStringValue(getConfigPropertyBasePath() + "storageLocation",
                                                getDSS().getDataDirectory() + "metering/",
                                                true,
                                                false);
    m_MeteringStorageLocation = getDSS().getPropertySystem().getStringValue(getConfigPropertyBasePath() + "storageLocation");
    m_MeteringStorageLocation = addTrailingBackslash(m_MeteringStorageLocation);

    if (getDSS().getPropertySystem().getBoolValue(getConfigPropertyBasePath() + "enabled")) {
      if (!boost::filesystem::is_directory(m_MeteringStorageLocation)) {
        throw std::runtime_error("Metering directory " + m_MeteringStorageLocation + " does not exist!");
      }

      if (!rwAccess(m_MeteringStorageLocation)) {
        throw std::runtime_error("Metering directory " + m_MeteringStorageLocation + " is not readable/writable!");
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

  void Metering::execute() {
    assert(m_pMeteringBusInterface != NULL);
    while (DSS::getInstance()->getModelMaintenance().isInitializing()) {
      sleepSeconds(1);
    }
    // check dsMeters periodically
    while (!m_Terminated) {
      int sleepTimeMSec = 60000 * 1000;

      try {
        checkDSMeters();
      } catch (dss::BusApiError& apiError) {
        log("Metering::execute: Couldn't get metering data: " + std::string(apiError.what()));
      }
      for (unsigned int iConfig = 0; iConfig < m_Config.size(); iConfig++) {
        sleepTimeMSec = std::min(sleepTimeMSec, 1000 * m_Config[iConfig]->getCheckIntervalSeconds());
      }
      while (!m_Terminated && (sleepTimeMSec > 0)) {
        const int kMinSleepTimeMS = 100;
        sleepMS(std::min(sleepTimeMSec, kMinSleepTimeMS));
        sleepTimeMSec -= kMinSleepTimeMS;
      }
    }
  } // execute

  boost::shared_ptr<std::string> Metering::getOrCreateCachedSeries(boost::shared_ptr<MeteringConfigChain> _pChain,
                                                                   boost::shared_ptr<DSMeter> _pMeter) {
    if (m_CachedSeries.find(_pMeter) == m_CachedSeries.end()) {
      std::string fileName = m_MeteringStorageLocation + _pMeter->getDSID().toString() + ".rrd";
      getDSS().getPropertySystem().setStringValue(getConfigPropertyBasePath() + "rrdDaemonAddress",
                                                  "unix:/var/run/rrdcached.sock", true, false);

      rrd_clear_error();
      rrd_info_t *rrdInfo = rrd_info_r((char *) fileName.c_str());
      if (rrdInfo != 0) {
        log("RRD DB present");
        /* TODO: check DB contents */
        rrd_info_free(rrdInfo);
      } else {
        /* create new DB */
        std::vector<std::string> lines;
        lines.push_back("DS:power:GAUGE:5:0:4500");
        lines.push_back("DS:energy:DERIVE:5:0:U");
        MeteringConfigChain *chain = _pChain.get();
        for (int i = 0; i < chain->size(); ++i) {
          std::stringstream sstream;
          sstream << "RRA:AVERAGE:0.5:" << chain->getResolution(i) << ":" << chain->getNumberOfValues(i);
          lines.push_back(sstream.str());
        }

        std::vector<const char*> starts;
        std::transform(lines.begin(), lines.end(), std::back_inserter(starts), boost::mem_fn(&std::string::c_str));
        const char** argString = &starts.front();

        DateTime iCurrentTimeStamp;
        rrd_clear_error();
        int result = rrd_create_r(fileName.c_str(),
                                  1,
                                  iCurrentTimeStamp.secondsSinceEpoch() - 10,
                                  starts.size(),
                                  argString);
        if (result < 0) {
          log(rrd_get_error());
          boost::shared_ptr<std::string> pFileName(new std::string(""));
          return pFileName;
        }
      }

      boost::shared_ptr<std::string> pFileName(new std::string(fileName));
      m_CachedSeries[_pMeter] = pFileName;
    }
    return m_CachedSeries[_pMeter];
  } // getOrCreateCachedSeries

  void Metering::postMeteringEvent(boost::shared_ptr<DSMeter> _meter,
                                   int _valuePower,
                                   int _valueEnergy,
                                   DateTime _sampledAt) {
    m_ValuesMutex.lock();
    boost::shared_ptr<std::string> rrdFileName = getOrCreateCachedSeries(m_ConfigConsumption, _meter);

    std::vector<std::string> lines;
    lines.push_back("update");
    lines.push_back("--daemon");
    lines.push_back(getDSS().getPropertySystem().getStringValue(getConfigPropertyBasePath() + "rrdDaemonAddress"));
    lines.push_back(rrdFileName.get()->c_str());
    {
      std::stringstream sstream;
      sstream << _sampledAt.secondsSinceEpoch() << ":" << _valuePower << ":" << _valueEnergy;
      lines.push_back(sstream.str());
    }
    std::vector<const char*> starts;
    std::transform(lines.begin(), lines.end(), std::back_inserter(starts), boost::mem_fn(&std::string::c_str));
    char** argString = (char **)&starts.front();

    rrd_clear_error();
    int result = rrd_update(starts.size(), argString);
    if (result < 0) {
      log(rrd_get_error());
    }
    m_ValuesMutex.unlock();
  } // postMeteringEvent

  boost::shared_ptr<std::deque<Value> > Metering::getSeries(boost::shared_ptr<DSMeter> _meter,
                                                            int &_resolution,
                                                            bool getEnergy) {
    m_ValuesMutex.lock();
    boost::shared_ptr<std::deque<Value> > returnVector(new std::deque<Value>);
    std::deque<Value>* values = returnVector.get();

    boost::shared_ptr<std::string> rrdFileName = getOrCreateCachedSeries(m_ConfigConsumption, _meter);
    DateTime iCurrentTimeStamp;
    long unsigned int step = _resolution;
    long unsigned int dscount = 0;
    time_t end = (iCurrentTimeStamp.secondsSinceEpoch() / step) * step;
    time_t start = end - (step * 399);
    char **names = 0;
    rrd_value_t *data = 0;

    {
      std::vector<std::string> lines;
      lines.push_back("flushcached");
      lines.push_back("--daemon");
      lines.push_back(getDSS().getPropertySystem().getStringValue(getConfigPropertyBasePath() + "rrdDaemonAddress"));
      lines.push_back(rrdFileName.get()->c_str());
      std::vector<const char*> starts;
      std::transform(lines.begin(), lines.end(), std::back_inserter(starts), boost::mem_fn(&std::string::c_str));
      char** argString = (char**)&starts.front();
      int result = rrd_flushcached(starts.size(), argString);
      if (result < 0) {
        log(rrd_get_error());
      }
    }

    std::vector<std::string> lines;
    lines.push_back("xport");
    lines.push_back("--daemon");
    lines.push_back("unix:/tmp/rrdcached.sock");
    lines.push_back("--start");
    lines.push_back(intToString(start));
    lines.push_back("--end");
    lines.push_back(intToString(end));
    lines.push_back("--step");
    lines.push_back(intToString(step));
    {
      std::stringstream sstream;
      sstream << "DEF:data=" << rrdFileName.get()->c_str() << ":" << (getEnergy ? "energy" : "power") << ":AVERAGE";
      lines.push_back(sstream.str());
    }
#if defined(ENERGY_IN_WS)
    if (getEnergy) {
      std::stringstream sstream;
      sstream << "CDEF:adj=data," << 3600 << ",*";
      lines.push_back(sstream.str());
      lines.push_back("XPORT:adj");
    } else {
#endif
      lines.push_back("XPORT:data");
#if defined(ENERGY_IN_WS)
    }
#endif

    std::vector<const char*> starts;
    std::transform(lines.begin(), lines.end(), std::back_inserter(starts), boost::mem_fn(&std::string::c_str));
    char** argString = (char**)&starts.front();
    rrd_clear_error();
    int result = rrd_xport(starts.size(),
                           argString,
                           0,
                           &start,
                           &end,
                           &step,
                           &dscount,
                           &names,
                           &data);
    if (result != 0) {
      log(rrd_get_error());
      m_ValuesMutex.unlock();
      return returnVector;
    }
    for (unsigned int i = 0; i < dscount; ++i) {
      rrd_freemem(names[i]);
    }
    rrd_freemem(names);
    rrd_value_t *currentData = data;
    for (int timeStamp = start + step; timeStamp <= end; timeStamp += step) {
      Value* val = new Value(std::isnan(*currentData) ? 0 : *currentData, timeStamp);
      currentData++;
      values->push_back(*val);
    }
    rrd_freemem(data);
    _resolution = step;
    m_ValuesMutex.unlock();
    return returnVector;
  }

  //================================================== MeteringConfigChain

  void MeteringConfigChain::addConfig(boost::shared_ptr<MeteringConfig> _config) {
    m_Chain.push_back(_config);
  } // addConfig

} // namespace dss
