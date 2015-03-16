/*
    Copyright (c) 2010,2011 digitalSTROM.org, Zurich, Switzerland

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

#include "meteringrequesthandler.h"

#include <vector>

#include "src/foreach.h"

#include "src/metering/metering.h"

#include "src/model/modulator.h"
#include "src/model/apartment.h"
#include "src/setbuilder.h"

namespace dss {


  //=========================================== MeteringRequestHandler

  MeteringRequestHandler::MeteringRequestHandler(Apartment& _apartment, Metering& _metering)
  : m_Apartment(_apartment),
    m_Metering(_metering)
  { }

  std::string MeteringRequestHandler::getResolutions() {
    std::vector<boost::shared_ptr<MeteringConfigChain> > meteringConfig = m_Metering.getConfig();
    JSONWriter json;
    json.startArray("resolutions");

    foreach(boost::shared_ptr<MeteringConfigChain> pChain, meteringConfig) {
      for(int iConfig = 0; iConfig < pChain->size(); iConfig++) {
        json.startObject();

        json.add("resolution", pChain->getResolution(iConfig));
        json.endObject();
      }
    }
    json.endArray();

    return json.successJSON();
  }

  std::string MeteringRequestHandler::getSeries() {
    JSONWriter json;
    json.startArray("series");

    std::vector<boost::shared_ptr<DSMeter> > dsMeters = m_Apartment.getDSMeters();
    foreach(boost::shared_ptr<DSMeter> dsMeter, dsMeters) {
      if (!dsMeter->getCapability_HasMetering()) {
        continue;
      }
      dsid_t dsid;
      // if there is no valid dsid we accept the all 0's one
      (void) dsuid_to_dsid(dsMeter->getDSID(), &dsid);

      json.startObject();
      json.add("dSUID", dsuid2str(dsMeter->getDSID()));
      json.add("dsid", dsid2str(dsid));
      json.add("type", "energy");
      json.endObject();

      json.startObject();
      json.add("dSUID", dsuid2str(dsMeter->getDSID()));
      json.add("dsid", dsid2str(dsid));
      json.add("type", "energyDelta");
      json.endObject();

      json.startObject();
      json.add("dSUID", dsuid2str(dsMeter->getDSID()));
      json.add("dsid", dsid2str(dsid));
      json.add("type", "consumption");
      json.endObject();
    }
    json.endArray();
    return json.successJSON();
  }

  std::string MeteringRequestHandler::getValues(const RestfulRequest& _request) {
    std::string deviceDSIDString = _request.getParameter("dsid");
    std::string deviceDSUIDString = _request.getParameter("dsuid");

    if (deviceDSIDString.empty() && deviceDSUIDString.empty()) {
      return JSONWriter::failure("Missing parameter 'dsuid'");
    }
    std::string deviceDSIDStringSet;
    bool requestMeteringSet = false;

    // set builder will handle dsid/dsuid compatibility
    if (deviceDSUIDString.empty()) {
      deviceDSUIDString = deviceDSIDString;
    }

    if (beginsWith(deviceDSUIDString, ".meters(")) {
      requestMeteringSet = true;
      deviceDSIDStringSet = deviceDSUIDString;
    } else {
      deviceDSIDStringSet = ".meters(" + deviceDSUIDString + ")";
    }
    std::string resolutionString = _request.getParameter("resolution");
    std::string typeString = _request.getParameter("type");
    std::string unitString = _request.getParameter("unit");
    std::string startTimeString = _request.getParameter("startTime");
    std::string endTimeString = _request.getParameter("endTime");
    std::string valueCountString = _request.getParameter("valueCount");
    int resolution;
    Metering::SeriesTypes energy;
    bool energyWh = false;
    MeterSetBuilder builder(m_Apartment);
    std::vector<boost::shared_ptr<DSMeter> > meters;
    try {
      meters = builder.buildSet(deviceDSIDStringSet);
    } catch(std::runtime_error& e) {
      return JSONWriter::failure(std::string("Couldn't parse parameter 'dsid': '") + e.what() + "'");
    }
    resolution = strToIntDef(resolutionString, -1);
    if(resolution == -1) {
      return JSONWriter::failure("Could not parse resolution '" + resolutionString + "'");
    }
    if(typeString.empty()) {
      return JSONWriter::failure("Need a type, 'energy', 'energyDelta' or 'consumption'");
    } else {
      if(typeString == "consumption") {
        energy = Metering::etConsumption;
        unitString = "W";
      } else if(typeString == "energyDelta") {
        energy = Metering::etEnergyDelta;
      } else if(typeString == "energy") {
        energy = Metering::etEnergy;
      } else {
        return JSONWriter::failure("Invalid type '" + typeString + "'");
      }

      if ((energy == Metering::etEnergyDelta) || (energy == Metering::etEnergy)) {
        if(unitString.empty()) {
          unitString = "Wh";
          energyWh = true;
        } else {
          if(unitString == "Ws") {
            energyWh = false;
          } else if (unitString == "Wh") {
            energyWh = true;
          } else {
            return JSONWriter::failure("Invalid unit '" + unitString + "'");
          }
        }
      }
    }
    DateTime startTime(DateTime::NullDate);
    DateTime endTime(DateTime::NullDate);
    int valueCount = 0;
    if (!startTimeString.empty()) {
      int timeStamp = strToIntDef(startTimeString, -1);
      if (timeStamp > -1) {
        startTime = DateTime(timeStamp);
      }
    }
    if (!endTimeString.empty()) {
      int timeStamp = strToIntDef(endTimeString, -1);
      if (timeStamp > -1) {
        endTime = DateTime(timeStamp);
      }
    }
    if (!valueCountString.empty()) {
      valueCount = strToIntDef(valueCountString, 0);
    }
    boost::shared_ptr<std::deque<Value> > pSeries = m_Metering.getSeries(meters,
                                                                         resolution,
                                                                         energy,
                                                                         energyWh,
                                                                         startTime,
                                                                         endTime,
                                                                         valueCount);

    if(pSeries != NULL) {
      JSONWriter json;
      if (requestMeteringSet) {
        json.startArray("meterID");
        for(std::vector<boost::shared_ptr<DSMeter> >::iterator iMeter = meters.begin(),
            e = meters.end();
            iMeter != e;
            ++iMeter)
        {
          std::string dsid = dsuid2str(iMeter->get()->getDSID());
          json.add(dsid);
        }
        json.endArray();
      } else {
        json.add("meterID", deviceDSUIDString);
      }
      json.add("type", typeString);
      json.add("unit", unitString);
      json.add("resolution", intToString(resolution));
      json.startArray("values");
      for(std::deque<Value>::iterator iValue = pSeries->begin(),
          e = pSeries->end();
          iValue != e;
          ++iValue)
      {
        json.startArray();
        DateTime tmp_date = iValue->getTimeStamp();
        json.add((int)tmp_date.secondsSinceEpoch());
        json.add(iValue->getValue());
        json.endArray();
      }
      json.endArray();

      return json.successJSON();
    } else {
      return JSONWriter::failure("Could not find data for '" + typeString + "' and resolution '" + resolutionString + "'");
    }
  }

  std::string MeteringRequestHandler::getLatest(const RestfulRequest& _request, bool aggregateMeterValues) {
    std::string from = _request.getParameter("from");
    std::string type = _request.getParameter("type");
    std::string unit = _request.getParameter("unit");

    if(type.empty() || ((type != "consumption") && (type != "energy"))) {
      return JSONWriter::failure("Invalid or missing type parameter");
    }

    if(from.empty()) {
      return JSONWriter::failure("Missing 'from' parameter");
    } else if (!beginsWith(from, ".meters(")) {
      from = ".meters(" + from + ")";
    }

    int energyQuotient = 3600;
    if(!unit.empty()) {
      if(unit == "Ws") {
        energyQuotient = 1;
      } else if(unit != "Wh") {
        return JSONWriter::failure("Invalid unit parameter");
      }
    }

    MeterSetBuilder builder(m_Apartment);
    std::vector<boost::shared_ptr<DSMeter> > meters;
    try {
      meters = builder.buildSet(from);
    } catch(std::runtime_error& e) {
      return JSONWriter::failure(std::string("Couldn't parse parameter 'from': '") + e.what() + "'");
    }

    JSONWriter json;
    json.startArray("values");

    bool isEnergy = (type == "energy");
    DateTime lastUpdateAll(DateTime::NullDate);
    unsigned long aggregatedValue = 0ul;
    std::vector<dsuid_t> dsuids;

    for(size_t i = 0; i < meters.size(); i++) {
      boost::shared_ptr<DSMeter> dsMeter = meters.at(i);
      double value = isEnergy ? (dsMeter->getCachedEnergyMeterValue() / energyQuotient) : dsMeter->getCachedPowerConsumption();
      DateTime lastUpdateGlobal = isEnergy ? dsMeter->getCachedEnergyMeterTimeStamp() : dsMeter->getCachedPowerConsumptionTimeStamp();
      if (lastUpdateGlobal > lastUpdateAll) {
        lastUpdateAll = lastUpdateGlobal;
      }
      std::string dsuid = dsuid2str(dsMeter->getDSID());
      dsid_t dsid;
      (void) dsuid_to_dsid(dsMeter->getDSID(), &dsid);
      if (aggregateMeterValues) {
        aggregatedValue += value;
        dsuids.push_back(dsMeter->getDSID());
      } else {
        try {
          json.startObject();
          json.add("dsid", dsid2str(dsid));
          json.add("dSUID", dsuid);
          json.add("value", value);
          json.add("date", lastUpdateAll.toString());
          json.endObject();
        } catch (std::runtime_error&) {
          return JSONWriter::failure("Could not apply properties to JSON object.");
        }
      }
    }
    if (aggregateMeterValues) {
      try {
        json.startObject();
        json.startArray("dsid");
        foreach(dsuid_t dsuid, dsuids) {
          dsid_t dsid;
          (void) dsuid_to_dsid(dsuid, &dsid);
          json.add(dsid2str(dsid));
        }
        json.endArray();
        json.startArray("dSUID");
        foreach(dsuid_t dsuid, dsuids) {
          json.add(dsuid2str(dsuid));
        }
        json.endArray();
        json.add("value", (long long int)aggregatedValue);
        json.add("date", lastUpdateAll.toString());
        json.endObject();
      } catch (std::runtime_error&) {
        return JSONWriter::failure("Could not apply properties to JSON object.");
      }
    }
    json.endArray();
    return json.successJSON();
  }

  WebServerResponse MeteringRequestHandler::jsonHandleRequest(const RestfulRequest& _request, boost::shared_ptr<Session> _session) {
    if(_request.getMethod() == "getResolutions") {
      return getResolutions();
    } else if(_request.getMethod() == "getSeries") {
      return getSeries();
    } else if(_request.getMethod() == "getValues") { //?dsid=;n=,resolution=,type=
      return getValues(_request);
    } else if(_request.getMethod() == "getLatest") {
      return getLatest(_request, false);
    } else if(_request.getMethod() == "getAggregatedValues") { //?set=;n=,resolution=;type=
      return getValues(_request);
    } else if(_request.getMethod() == "getAggregatedLatest") { //?dsid=;n=,resolution=;type=
      return getLatest(_request, true);
    }
    throw std::runtime_error("Unhandled function");
  } // handleRequest

} // namespace dss
