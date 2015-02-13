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

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif


#include "metering.h"

#include <boost/filesystem.hpp>
#include <digitalSTROM/dsuid/dsuid.h>

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
#include "foreach.h"
#include <functional>
#include <iterator>
#include <time.h>

#include <rrd.h>

#include <sys/types.h>
#include <regex.h>

#ifdef LOG_TIMING
  #include <sstream>
#endif


namespace dss {
  //================================================== Metering

static const long WEEK_IN_SECS = 604800;

  Metering::Metering(DSS* _pDSS)
    : ThreadedSubsystem(_pDSS, "Metering")
    , m_pMeteringBusInterface(NULL) {
    m_ConfigChain.reset(new MeteringConfigChain(1));
    m_ConfigChain->addConfig(MeteringConfig(                1,  600));
    m_ConfigChain->addConfig(MeteringConfig(               60,  720));
    m_ConfigChain->addConfig(MeteringConfig(          15 * 60, 2976));
    m_ConfigChain->addConfig(MeteringConfig(     24 * 60 * 60,  370));
    m_ConfigChain->addConfig(MeteringConfig( 7 * 24 * 60 * 60,  260));
    m_ConfigChain->addConfig(MeteringConfig(30 * 24 * 60 * 60,   60));
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
        throw std::runtime_error("Metering directory " + boost::filesystem::system_complete(m_MeteringStorageLocation).string() + " does not exist!");
      }

      if (!rwAccess(m_MeteringStorageLocation)) {
        throw std::runtime_error("Metering directory " + boost::filesystem::system_complete(m_MeteringStorageLocation).string() + " is not readable/writable!");
      }
    }
  }
  void Metering::doStart() {
    log("Writing files to: " + m_MeteringStorageLocation);
    run();
  } // start

  void Metering::execute() {
    assert(m_pMeteringBusInterface != NULL);
    while (DSS::getInstance()->getModelMaintenance().isInitializing()) {
      sleepSeconds(1);
    }
    // check dsMeters periodically
    while (!m_Terminated) {
      try {
        m_pMeteringBusInterface->requestMeterData();
      } catch (dss::BusApiError& apiError) {
        log("Metering::execute: Couldn't get metering data: " + std::string(apiError.what()));
      }

      int sleepTimeMSec = 60000 * 1000;
      foreach (boost::shared_ptr<MeteringConfigChain> configChain, m_Config) {
        sleepTimeMSec = std::min(sleepTimeMSec, 1000 * configChain->getCheckIntervalSeconds());
      }

      while (!m_Terminated && (sleepTimeMSec > 0)) {
        const int kMinSleepTimeMS = 100;
        sleepMS(std::min(sleepTimeMSec, kMinSleepTimeMS));
        sleepTimeMSec -= kMinSleepTimeMS;
      }
    }
  } // execute

  bool Metering::validateDBSeries(std::string& _fileName,
                                  boost::shared_ptr<MeteringConfigChain> _pChain,
                                  bool& _tunePowerMaxSetting) {
    int rrdMatchCount = 0;
    rrd_clear_error();
    rrd_info_t *rrdInfo = rrd_info_r((char *) _fileName.c_str());
    if (rrdInfo != 0) {
      regex_t rraRowRegex;
      int regCompErr = regcomp(&rraRowRegex, "rra\\[([0-9])\\].rows", REG_EXTENDED);
      if (regCompErr != 0) {
        int errSize = regerror(regCompErr, &rraRowRegex, 0, 0);
        char *errString = new char[errSize];
        regerror(regCompErr, &rraRowRegex, errString, errSize);
        log(errString, lsError);
        delete [] errString;
      }
      rrd_info_t *rrdInfoOrig = rrdInfo;
      log("RRD DB present");
      /* check DB contents */
      while (rrdInfo != NULL) {
        if (((strcmp(rrdInfo->key, "ds[power].index") == 0) &&
             (rrdInfo->type == RD_I_CNT) &&
             (rrdInfo->value.u_cnt == 0)) ||
            ((strcmp(rrdInfo->key, "ds[energy].index") == 0) &&
             (rrdInfo->type == RD_I_CNT) &&
             (rrdInfo->value.u_cnt == 1))) {
          rrdMatchCount++;
        } else {
          regmatch_t subMatch[2];
          if (regexec(&rraRowRegex, rrdInfo->key, 2, subMatch, 0) == 0) {
            std::string key(rrdInfo->key);
            int index = strToInt(key.substr(subMatch[1].rm_so, subMatch[1].rm_eo - subMatch[1].rm_so));
            if (static_cast<int>(rrdInfo->value.u_cnt) == _pChain->getNumberOfValues(index)) {
              rrdMatchCount++;
            }
          }
        }

        if ((strcmp(rrdInfo->key, "ds[power].max") == 0) &&
            (rrdInfo->type == RD_I_VAL) &&
            (rrdInfo->value.u_val == 4.5e3)) {
          _tunePowerMaxSetting = true;
        }

        rrdInfo = rrdInfo->next;
      }
      rrd_info_free(rrdInfoOrig);
      regfree(&rraRowRegex);
    }
    log("RRD MatchCount: " + intToString(rrdMatchCount));
    return (rrdMatchCount == (2 + _pChain->size()));
  }

  void Metering::tuneDBPowerSettings(std::string& _fileName)
  {
    log(std::string("tuning max acceptable power value in RRD ") + _fileName, lsWarning);
    std::vector<std::string> lines;
    lines.push_back("tune");
    lines.push_back(_fileName);
    lines.push_back("--maximum");
    lines.push_back("power:40000");

    std::vector<const char*> starts;
    std::transform(lines.begin(), lines.end(), std::back_inserter(starts), boost::mem_fn(&std::string::c_str));
    const char** argString = &starts.front();
    rrd_clear_error();
    // cast-away the const, should be ok according to rrd_tune() sources
    int result = rrd_tune(starts.size(), (char**)argString);
    if (result < 0) {
      log(rrd_get_error());
    }
  }

  boost::shared_ptr<std::string> Metering::getOrCreateCachedSeries(boost::shared_ptr<MeteringConfigChain> _pChain,
                                                                   boost::shared_ptr<DSMeter> _pMeter) {
    if (m_CachedSeries.find(_pMeter) != m_CachedSeries.end()) {
      return m_CachedSeries[_pMeter];
    }

    std::string fileName = m_MeteringStorageLocation + dsuid2str(_pMeter->getDSID()) + ".rrd";

    if (!boost::filesystem::exists(fileName)) {
      dsuid_t dsuid = _pMeter->getDSID();
      dsid_t dsid;
      if (::dsuid_to_dsid(&dsuid, &dsid) == DSUID_RC_OK) {
        std::string oldFile = m_MeteringStorageLocation + dsid2str(dsid) +
                              ".rrd";
        if (boost::filesystem::exists(oldFile)) {
          boost::filesystem::rename(oldFile, fileName);
          log("Migrated metering data from " + oldFile + " to " + fileName);
        }
      }
    }

    bool tunePowerMaxSetting = false;
    bool validate = validateDBSeries(fileName, _pChain, tunePowerMaxSetting);

    if (!validate) {
      int result = createDB(fileName, _pChain);
      if (result < 0) {
        log(rrd_get_error(), lsError);
        boost::shared_ptr<std::string> pFileName = boost::make_shared<std::string>("");
        return pFileName;
      }
    } else if (tunePowerMaxSetting) {
      tuneDBPowerSettings(fileName);
    }

    boost::shared_ptr<std::string> pFileName = boost::make_shared<std::string>(fileName);
    m_CachedSeries[_pMeter] = pFileName;
    return m_CachedSeries[_pMeter];
  } // getOrCreateCachedSeries

  void Metering::postMeteringEvent(boost::shared_ptr<DSMeter> _meter,
                                   unsigned int _valuePower,
                                   unsigned long long _valueEnergy,
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

  int Metering::createDB(std::string& _filename, boost::shared_ptr<MeteringConfigChain> _pChain)
  {
    log("Creating new RRD database.", lsWarning);

    /* create new DB */
    std::vector<std::string> lines;
    lines.push_back("DS:power:GAUGE:5:0:40000");
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
    time_t currentInSecs = iCurrentTimeStamp.secondsSinceEpoch();
    if (currentInSecs > 10) {
      currentInSecs -= 10;
    }

    rrd_clear_error();
    int result = rrd_create_r(_filename.c_str(),
                              1,
                              currentInSecs,
                              starts.size(),
                              argString);

    return result;
  }

 void Metering::flushCachedDBValues(boost::shared_ptr<std::string> _rrdFileName) {
    std::vector<boost::shared_ptr<std::string> > rrdFileNames;
    rrdFileNames.push_back(_rrdFileName);
    flushCachedDBValues(rrdFileNames);
  }

  void Metering::flushCachedDBValues(std::vector<boost::shared_ptr<std::string> > _rrdFileNames) {
    std::vector<std::string> lines;
    lines.push_back("flushcached");
    lines.push_back("--daemon");
    lines.push_back(m_RrdcachedPath);
    for (std::vector<boost::shared_ptr<std::string> >::iterator iter = _rrdFileNames.begin();
         iter < _rrdFileNames.end();
         ++iter) {
      lines.push_back(iter->get()->c_str());
    }
    std::vector<const char*> starts;
    std::transform(lines.begin(), lines.end(), std::back_inserter(starts), boost::mem_fn(&std::string::c_str));
    char** argString = (char**)&starts.front();
    log("flushing cached rrd data to file", lsInfo);
    int result = rrd_flushcached(starts.size(), argString);
    if (result < 0) {
      log(rrd_get_error());
    }
  }

  unsigned long Metering::getLastEnergyCounter(boost::shared_ptr<DSMeter> _meter) {
    m_ValuesMutex.lock();
    boost::shared_ptr<std::string> rrdFileName = getOrCreateCachedSeries(m_ConfigChain, _meter);


    if (!m_RrdcachedPath.empty()) {

      // Get last entry-timestamp of data in file
      time_t  timestamp = rrd_last_r(rrdFileName.get()->c_str());

      // Get current timestamp
      time_t actualTime;
      time(&actualTime);

      // Calculate absolute delta
      double secondsAbs = std::abs(difftime(actualTime, timestamp));

      log("Actual Timestamp:"+ doubleToString(actualTime) +
          "RRD Last Entry Timestamp:" + doubleToString(timestamp), lsDebug);

      // call flushCachedDBValues (blocking function) only if delta is small enough.
      if (secondsAbs < WEEK_IN_SECS) {
        flushCachedDBValues(rrdFileName);
      } else {
        log("Time difference for rrd data too big. Not flushing cached data to file.", lsWarning);
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
      if (strcmp(names[i], "energy") == 0) {
        lastEnergyCounter = strToUIntDef(data[i], 0);
      }
      rrd_freemem(names[i]);
      rrd_freemem(data[i]);
    }
    rrd_freemem(names);
    rrd_freemem(data);

    return lastEnergyCounter;
  }

  boost::shared_ptr<std::deque<Value> > Metering::getSeries(std::vector<boost::shared_ptr<DSMeter> > _meters,
                                                            int &_resolution,
                                                            SeriesTypes _type,
                                                            bool _energyInWh,
                                                            DateTime &_startTime,
                                                            DateTime &_endTime,
                                                            int &_valueCount) {
    if (_meters.size() == 0) {
      _meters = getDSS().getApartment().getDSMeters();
    }

    int numberOfValues = 0;
    for (int i = 0; i < m_ConfigChain->size(); ++i) {
      if (m_ConfigChain->getResolution(i) <= _resolution) {
        numberOfValues = m_ConfigChain->getNumberOfValues(i);
      }
    }

    boost::shared_ptr<std::deque<Value> > returnVector = boost::make_shared<std::deque<Value> >();
    if (_meters.size() == 0) {
      _valueCount = 0;
      return returnVector;
    }
    std::vector<boost::shared_ptr<std::string> > rrdFileNames;

    for (std::vector<boost::shared_ptr<DSMeter> >::iterator iter = _meters.begin();
         iter < _meters.end();
         ++iter) {
      if (! (*iter)->getCapability_HasMetering()) {
        continue;
      }
      rrdFileNames.push_back(getOrCreateCachedSeries(m_ConfigChain, *iter));
    }

    DateTime iCurrentTimeStamp;
    long unsigned int step = _resolution;
    time_t end = (iCurrentTimeStamp.secondsSinceEpoch() / step) * step;
    time_t start = end - (step * numberOfValues);
    _valueCount = std::min(_valueCount, numberOfValues);

    if ((_startTime == DateTime::NullDate) && (_endTime == DateTime::NullDate) && (_valueCount == 0)) {
      // now-(step*numberOfValues)..now
    } else if ((_startTime == DateTime::NullDate) && (_endTime == DateTime::NullDate) && (_valueCount != 0)) {
      // now-(step*valueCount)..now
      start = end - (step * _valueCount);
    } else if ((_startTime == DateTime::NullDate) && (_endTime != DateTime::NullDate) && (_valueCount == 0)) {
      // now-(step*numberOfValues)..endTime
      end = (_endTime.secondsSinceEpoch() / step) * step;
    } else if ((_startTime == DateTime::NullDate) && (_endTime != DateTime::NullDate) && (_valueCount != 0)) {
      // endTime-(step*valueCount)..endTime
      end = (_endTime.secondsSinceEpoch() / step) * step;
      start = end - (step * _valueCount);
    } else if ((_startTime != DateTime::NullDate) && (_endTime == DateTime::NullDate) && (_valueCount == 0)) {
      // startTime..now
      start = (_startTime.secondsSinceEpoch() / step) * step;
    } else if ((_startTime != DateTime::NullDate) && (_endTime == DateTime::NullDate) && (_valueCount != 0)) {
      // startTime..startTime+(step*valueCount)
      start = (_startTime.secondsSinceEpoch() / step) * step;
      end = start + (step * _valueCount);
    } else if ((_startTime != DateTime::NullDate) && (_endTime != DateTime::NullDate)) {
      // startTime..endTime
      start = (_startTime.secondsSinceEpoch() / step) * step;
      end = (_endTime.secondsSinceEpoch() / step) * step;
    }

    m_ValuesMutex.lock();

    if (!m_RrdcachedPath.empty()) {
      flushCachedDBValues(rrdFileNames);
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
      int i = 0;
      int size = rrdFileNames.size();
      std::stringstream sstream;
      for (i = 0; i < size; ++i) {
        const char* filename = rrdFileNames.at(i)->c_str();
        sstream << "DEF:raw" << i << "=" << filename;
        if ((_type == etConsumption) && (step == 1)) {
          sstream << ":power";
        } else {
          sstream << ":energy";
        }
        sstream << ":AVERAGE";
        lines.push_back(sstream.str());

        sstream.str(std::string());
        sstream << "CDEF:data" << i << "=raw" << i << ",UN,0,raw" << i << ",IF";
        lines.push_back(sstream.str());
        sstream.str(std::string());
      }
      sstream << "CDEF:sum=data0";
      for (i = 1; i < size; ++i) {
        sstream << ",data" << i << ",+";
      }
      lines.push_back(sstream.str());
    }
    lines.push_back("CDEF:limit=sum,0,40000,LIMIT");
    switch (_type) {
    case etEnergy:
    case etEnergyDelta: {
      lines.push_back("CDEF:ratePerStep=limit," + intToString(step) + ",*");
      if (_energyInWh) {
        lines.push_back("CDEF:adj=ratePerStep,3600,/");
        lines.push_back("XPORT:adj");
      } else {
        lines.push_back("XPORT:ratePerStep");
      }
      break;
    }
    default: {
      lines.push_back("XPORT:limit");
      break;
    }
    }

    std::vector<const char*> starts;
    std::transform(lines.begin(), lines.end(), std::back_inserter(starts), boost::mem_fn(&std::string::c_str));
    char** argString = (char**)&starts.front();

    unsigned long dscount = 0;
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
    for (unsigned long i = 0; i < dscount; ++i) {
      rrd_freemem(names[i]);
    }
    rrd_freemem(names);
    rrd_value_t *currentData = data;
    for (time_t timeStamp = start + step; timeStamp <= (time_t)(end - step); timeStamp += step) {
      returnVector->push_back(Value(*currentData, DateTime(timeStamp)));
      currentData++;
    }
    bool lastValueEmpty = false;
    if ((returnVector->size() > 0) && (returnVector->back().getValue() == 0)) {
      lastValueEmpty = true;
    }

    if (_type == etEnergy) {
      double currentCounter = 0.0;
      for (std::vector<boost::shared_ptr<DSMeter> >::iterator iter = _meters.begin();
           iter < _meters.end();
           ++iter) {
        currentCounter += (*iter)->getCachedEnergyMeterValue();
      }
      if (_energyInWh) {
        currentCounter /= 3600;
      }
      Value lastZeroValue(0, DateTime((time_t)0));
      for (std::deque<Value>::iterator iter = returnVector->begin();
           iter < returnVector->end();
           ++iter) {
        if (iter->getValue() == 0) {
          lastZeroValue = *iter;
        } else {
          break;
        }
      }
      for (std::deque<Value>::reverse_iterator iter = returnVector->rbegin();
           iter < returnVector->rend();
           ++iter) {
        if (iter->getTimeStamp() == lastZeroValue.getTimeStamp()) {
          break;
        }
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
    _startTime = DateTime(start);
    _endTime = DateTime(end);
    return returnVector;
  }

  //================================================== MeteringConfigChain

  MeteringConfigChain::MeteringConfigChain(int _checkIntervalSeconds)
  : m_CheckIntervalSeconds(_checkIntervalSeconds)
  { }

  MeteringConfigChain::~MeteringConfigChain()
  {
    m_Chain.clear();
  }

  void MeteringConfigChain::addConfig(MeteringConfig _config) {
    m_Chain.push_back(_config);
  }

  const int MeteringConfigChain::size() const {
    return m_Chain.size();
  }

  const int MeteringConfigChain::getResolution(int _index) const {
    return m_Chain.at(_index).m_Resolution;
  }

  const int MeteringConfigChain::getNumberOfValues(int _index) const {
    return m_Chain.at(_index).m_NumberOfValues;
  }

  int MeteringConfigChain::getCheckIntervalSeconds() const {
    return m_CheckIntervalSeconds;
  }

} // namespace dss
