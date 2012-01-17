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

#include "src/dss.h"
#include "src/logger.h"
#include "src/businterface.h"
#include "src/propertysystem.h"
#include "src/foreach.h"
#include "src/model/modulator.h"
#include "src/model/apartment.h"
#include "src/model/modelmaintenance.h"
#include "src/security/security.h"

#include <algorithm>
#include <functional>
#include <iterator>

#include <rrd.h>

#ifdef LOG_TIMING
  #include <sstream>
#endif

namespace dss {

static const unsigned long kRRDHeaderSize = 2220;
  //================================================== Metering

  Metering::Metering(DSS* _pDSS)
    : ThreadedSubsystem(_pDSS, "Metering")
    , m_pMeteringBusInterface(NULL) {
    m_ConfigChain.reset(new MeteringConfigChain(1));
    m_ConfigChain->addConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig(                1,  600)));
    m_ConfigChain->addConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig(               60,  720)));
    m_ConfigChain->addConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig(          15 * 60, 2976)));
    m_ConfigChain->addConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig(     24 * 60 * 60,  370)));
    m_ConfigChain->addConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig( 7 * 24 * 60 * 60,  260)));
    m_ConfigChain->addConfig(boost::shared_ptr<MeteringConfig>(new MeteringConfig(30 * 24 * 60 * 60,   60)));
    m_Config.push_back(m_ConfigChain);
  } // metering

  void Metering::initialize() {
    Subsystem::initialize();
    getDSS().getPropertySystem().setStringValue(getConfigPropertyBasePath() + "storageLocation",
                                                getDSS().getDataDirectory() + "metering/",
                                                true,
                                                false);
    m_MeteringStorageLocation = getDSS().getPropertySystem().getStringValue(getConfigPropertyBasePath() + "storageLocation");
    m_MeteringStorageLocation = addTrailingBackslash(m_MeteringStorageLocation);
    getDSS().getPropertySystem().setStringValue(getConfigPropertyBasePath() + "rrdDaemonAddress",
                                                "unix:/var/run/rrdcached.sock", true, false);
    m_RrdcachedPath = getDSS().getPropertySystem().getStringValue(getConfigPropertyBasePath() + "rrdDaemonAddress");

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
      bool rrdPresent = false;
      std::string fileName = m_MeteringStorageLocation + _pMeter->getDSID().toString() + ".rrd";

      rrd_clear_error();
      rrd_info_t *rrdInfo = rrd_info_r((char *) fileName.c_str());
      if (rrdInfo != 0) {
        log("RRD DB present");
        /* check DB contents */
        while (rrdInfo != NULL) {
          if ((rrdInfo->type == RD_I_CNT) && (strcmp(rrdInfo->key, "header_size") == 0)) {
            unsigned long headerSize = rrdInfo->value.u_cnt;
            log("RRD Header Size: " + intToString(headerSize));
            if (headerSize == kRRDHeaderSize) {
              rrdPresent = true;
              log("RRD Header Size matches!");
              break;
            }
          }
          rrdInfo = rrdInfo->next;
        }
        rrd_info_free(rrdInfo);
      }

      if (!rrdPresent) {
        log("Creating new RRD database.", lsWarning);
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
          log(rrd_get_error(), lsError);
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
    boost::shared_ptr<std::string> rrdFileName = getOrCreateCachedSeries(m_ConfigChain, _meter);

    std::vector<std::string> lines;
    lines.push_back("update");
    if (!m_RrdcachedPath.empty()) {
      lines.push_back("--daemon");
      lines.push_back(m_RrdcachedPath);
    }
    lines.push_back(rrdFileName.get()->c_str());
    {
      std::stringstream sstream;
      sstream << _sampledAt.secondsSinceEpoch() << ":" << _valuePower << ":" << _valueEnergy;
      lines.push_back(sstream.str());
    }
    std::vector<const char*> starts;
    std::transform(lines.begin(), lines.end(), std::back_inserter(starts), boost::mem_fn(&std::string::c_str));
    char** argString = (char **)&starts.front();

    m_ValuesMutex.lock();

    rrd_clear_error();
    int result = rrd_update(starts.size(), argString);
    if (result < 0) {
      log(rrd_get_error());
    }
    m_ValuesMutex.unlock();
  } // postMeteringEvent

  unsigned long Metering::getLastEnergyCounter(boost::shared_ptr<DSMeter> _meter) {
    m_ValuesMutex.lock();
    boost::shared_ptr<std::string> rrdFileName = getOrCreateCachedSeries(m_ConfigChain, _meter);

    if (!m_RrdcachedPath.empty()) {
      std::vector<std::string> lines;
      lines.push_back("flushcached");
      lines.push_back("--daemon");
      lines.push_back(m_RrdcachedPath);
      lines.push_back(rrdFileName.get()->c_str());
      std::vector<const char*> starts;
      std::transform(lines.begin(), lines.end(), std::back_inserter(starts), boost::mem_fn(&std::string::c_str));
      char** argString = (char**)&starts.front();
      int result = rrd_flushcached(starts.size(), argString);
      if (result < 0) {
        log(rrd_get_error());
      }
    }

    char **names = 0;
    char **data = 0;
    time_t lastUpdate;
    unsigned long dscount;
    rrd_clear_error();
    int result = rrd_lastupdate_r(rrdFileName.get()->c_str(),
                                  &lastUpdate,
                                  &dscount,
                                  &names,
                                  &data);

    m_ValuesMutex.unlock();

    if (result != 0) {
      log(rrd_get_error());
      return 0;
    }

    unsigned long lastEnergyCounter = 0;
    for (unsigned int i = 0; i < dscount; ++i) {
      if (i == 1) {
        lastEnergyCounter = strToUIntDef(data[i], 0);
      }
      rrd_freemem(names[i]);
      rrd_freemem(data[i]);
    }
    rrd_freemem(names);
    rrd_freemem(data);

    return lastEnergyCounter;
  }

  boost::shared_ptr<std::deque<Value> > Metering::getSeries(boost::shared_ptr<DSMeter> _meter,
                                                            int &_resolution,
                                                            SeriesTypes _type,
                                                            bool _energyInWh) {
    int numberOfValues = 0;
    for (int i = 0; i < m_ConfigChain->size(); ++i) {
      if (m_ConfigChain->getResolution(i) <= _resolution) {
        numberOfValues = m_ConfigChain->getNumberOfValues(i);
      }
    }

    boost::shared_ptr<std::deque<Value> > returnVector(new std::deque<Value>);
    boost::shared_ptr<std::string> rrdFileName = getOrCreateCachedSeries(m_ConfigChain, _meter);

    DateTime iCurrentTimeStamp;
    long unsigned int step = _resolution;
    time_t end = (iCurrentTimeStamp.secondsSinceEpoch() / step) * step;
    time_t start = end - (step * numberOfValues);

    m_ValuesMutex.lock();

    if (!m_RrdcachedPath.empty()) {
      std::vector<std::string> lines;
      lines.push_back("flushcached");
      lines.push_back("--daemon");
      lines.push_back(m_RrdcachedPath);
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
    if (!m_RrdcachedPath.empty()) {
      lines.push_back("--daemon");
      lines.push_back(m_RrdcachedPath);
    }
    lines.push_back("--start");
    lines.push_back(intToString(start));
    lines.push_back("--end");
    lines.push_back(intToString(end));
    lines.push_back("--step");
    lines.push_back(intToString(step));
    lines.push_back("--maxrows");
    lines.push_back("3000");
    {
      std::stringstream sstream;
      sstream << "DEF:data=" << rrdFileName.get()->c_str() << ":" << ((_type == etConsumption) ? "power" : "energy") << ":AVERAGE";
      lines.push_back(sstream.str());
    }
    lines.push_back("CDEF:noUnkn=data,UN,0,data,IF");
    switch (_type) {
    case etEnergy:
    case etEnergyDelta: {
      lines.push_back("CDEF:ratePerStep=noUnkn," + intToString(step) + ",*");
      if (_energyInWh) {
        lines.push_back("CDEF:adj=ratePerStep,3600,/");
        lines.push_back("XPORT:adj");
      } else {
        lines.push_back("XPORT:ratePerStep");
      }
      break;
    }
    default: {
      lines.push_back("XPORT:noUnkn");
      break;
    }
    }

    std::vector<const char*> starts;
    std::transform(lines.begin(), lines.end(), std::back_inserter(starts), boost::mem_fn(&std::string::c_str));
    char** argString = (char**)&starts.front();

    long unsigned int dscount = 0;
    char **names = 0;
    rrd_value_t *data = 0;

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

    m_ValuesMutex.unlock();

    if (result != 0) {
      log(rrd_get_error());
      return returnVector;
    }
    for (unsigned int i = 0; i < dscount; ++i) {
      rrd_freemem(names[i]);
    }
    rrd_freemem(names);
    rrd_value_t *currentData = data;
    for (int timeStamp = start + step; timeStamp <= (end - step); timeStamp += step) {
      returnVector->push_back(Value(*currentData, DateTime(timeStamp)));
      currentData++;
    }
    bool lastValueEmpty = false;
    if (returnVector->back().getValue() == 0) {
      lastValueEmpty = true;
    }

    if (_type == etEnergy) {
      double currentCounter = _meter->getCachedEnergyMeterValue();
      if (_energyInWh) {
        currentCounter /= 3600;
      }
      for (std::deque<Value>::reverse_iterator iter = returnVector->rbegin();
           iter < returnVector->rend();
           ++iter) {
        double val = iter->getValue();
        iter->setValue(currentCounter);
        currentCounter -= val;
      }
    }

    if (lastValueEmpty) {
      // delete the last value, if it is Unknown (zero)
      returnVector->pop_back();
    }
    rrd_freemem(data);
    _resolution = step;
    return returnVector;
  }

  //================================================== MeteringConfigChain

  void MeteringConfigChain::addConfig(boost::shared_ptr<MeteringConfig> _config) {
    m_Chain.push_back(_config);
  } // addConfig

} // namespace dss
