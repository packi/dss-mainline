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

#include "core/foreach.h"

#include "core/metering/metering.h"

#include "core/model/modulator.h"
#include "core/model/apartment.h"
#include "core/setbuilder.h"


#include "core/web/json.h"

namespace dss {


  //=========================================== MeteringRequestHandler

  MeteringRequestHandler::MeteringRequestHandler(Apartment& _apartment, Metering& _metering)
  : m_Apartment(_apartment),
    m_Metering(_metering)
  { }

  boost::shared_ptr<JSONObject> MeteringRequestHandler::getResolutions() {
    std::vector<boost::shared_ptr<MeteringConfigChain> > meteringConfig = m_Metering.getConfig();
    boost::shared_ptr<JSONObject> resultObj(new JSONObject());
    boost::shared_ptr<JSONArrayBase> resolutions(new JSONArrayBase());
    resultObj->addElement("resolutions", resolutions);

    foreach(boost::shared_ptr<MeteringConfigChain> pChain, meteringConfig) {
      for(int iConfig = 0; iConfig < pChain->size(); iConfig++) {
        boost::shared_ptr<JSONObject> resolution(new JSONObject());
        resolutions->addElement("", resolution);

        resolution->addProperty("resolution", pChain->getResolution(iConfig));
      }
    }

    return success(resultObj);
  }

  boost::shared_ptr<JSONObject> MeteringRequestHandler::getSeries() {
    boost::shared_ptr<JSONObject> resultObj(new JSONObject());
    boost::shared_ptr<JSONArrayBase> series(new JSONArrayBase());
    resultObj->addElement("series", series);

    std::vector<boost::shared_ptr<DSMeter> > dsMeters = m_Apartment.getDSMeters();
    foreach(boost::shared_ptr<DSMeter> dsMeter, dsMeters) {
      boost::shared_ptr<JSONObject> energyEntry(new JSONObject());
      series->addElement("", energyEntry);
      energyEntry->addProperty("dsid", dsMeter->getDSID().toString());
      energyEntry->addProperty("type", "energy");
      boost::shared_ptr<JSONObject> consumptionEntry(new JSONObject());
      series->addElement("", consumptionEntry);
      consumptionEntry->addProperty("dsid", dsMeter->getDSID().toString());
      consumptionEntry->addProperty("type", "consumption");
    }
    return success(resultObj);
  }

  boost::shared_ptr<JSONObject> MeteringRequestHandler::getValues(const RestfulRequest& _request) {
    std::string deviceDSIDString = _request.getParameter("dsid");
    std::string resolutionString = _request.getParameter("resolution");
    std::string typeString = _request.getParameter("type");
    std::string unitString = _request.getParameter("unit");
    int resolution;
    bool energy;
    bool energyWh;
    boost::shared_ptr<DSMeter> pMeter;
    try {
      dss_dsid_t deviceDSID = dss_dsid_t::fromString(deviceDSIDString);
      try {
        pMeter = m_Apartment.getDSMeterByDSID(deviceDSID);
      } catch(std::runtime_error& e) {
        return failure("Could not find device with dsid '" + deviceDSIDString + "'");
      }
    } catch(std::runtime_error& e) {
      return failure("Could not parse dsid '" + deviceDSIDString + "'");
    } catch(std::invalid_argument& e) {
      return failure("Could not parse dsid '" + deviceDSIDString + "'");
    }
    resolution = strToIntDef(resolutionString, -1);
    if(resolution == -1) {
      return failure("Could not parse resolution '" + resolutionString + "'");
    }
    if(typeString.empty()) {
      return failure("Need a type, 'energy' or 'consumption'");
    } else {
      if(typeString == "consumption") {
        energy = false;
        energyWh = true;
        unitString = "W";
      } else if(typeString == "energy") {
        energy = true;
        if(unitString.empty()) {
          unitString = "Wh";
          energyWh = true;
        } else {
          if(unitString == "Ws") {
            energyWh = false;
          } else {
            return failure("Invalid unit '" + unitString + "'");
          }
        }
      } else {
        return failure("Invalid type '" + typeString + "'");
      }
    }
    boost::shared_ptr<std::deque<Value> > pSeries = m_Metering.getSeries(pMeter, resolution, energy, energyWh);

    if(pSeries != NULL) {
      boost::shared_ptr<JSONObject> resultObj(new JSONObject());
      resultObj->addProperty("meterID", deviceDSIDString);
      resultObj->addProperty("type", typeString);
      resultObj->addProperty("unit", unitString);
      resultObj->addProperty("resolution", resolutionString);
      boost::shared_ptr<JSONArrayBase> valuesArray(new JSONArrayBase());
      resultObj->addElement("values", valuesArray);
      for(std::deque<Value>::iterator iValue = pSeries->begin(),
          e = pSeries->end();
          iValue != e;
          ++iValue)
      {
        boost::shared_ptr<JSONArrayBase> valuePair(new JSONArrayBase());
        valuesArray->addElement("", valuePair);
        DateTime tmp_date = iValue->getTimeStamp();
        boost::shared_ptr<JSONValue<int> > timeVal(new JSONValue<int>(tmp_date.secondsSinceEpoch()));
        boost::shared_ptr<JSONValue<double> > valueVal(new JSONValue<double>(iValue->getValue()));
        valuePair->addElement("", timeVal);
        valuePair->addElement("", valueVal);
      }

      return success(resultObj);
    } else {
      return failure("Could not find data for '" + typeString + "' and resolution '" + resolutionString + "'");
    }
  }

  boost::shared_ptr<JSONObject> MeteringRequestHandler::getLatest(const RestfulRequest& _request) {
    std::string from = _request.getParameter("from");
    std::string type = _request.getParameter("type");
    std::string unit = _request.getParameter("unit");

    if(type.empty() || ((type != "consumption") && (type != "energy"))) {
      return failure("Invalid or missing type parameter");
    }

    if(from.empty()) {
      return failure("Missing 'from' parameter");
    }

    int energyQuotient = 3600;
    if(!unit.empty()) {
      if(unit == "Ws") {
        energyQuotient = 1;
      } else if(unit != "Wh") {
        return failure("Invalid unit parameter");
      }
    }

    MeterSetBuilder builder(m_Apartment);
    std::vector<boost::shared_ptr<DSMeter> > meters;
    try {
      meters = builder.buildSet(from);
    } catch(std::runtime_error& e) {
      return failure(std::string("Couldn't parse parameter 'from': '") + e.what() + "'");
    }

    boost::shared_ptr<JSONObject> resultObj(new JSONObject());
    boost::shared_ptr<JSONArrayBase> modulators(new JSONArrayBase());
    resultObj->addElement("values", modulators);

    bool isEnergy = (type == "energy");

    for(size_t i = 0; i < meters.size(); i++) {
      try {
        boost::shared_ptr<DSMeter> dsMeter = meters.at(i);
        boost::shared_ptr<JSONObject> modulator(new JSONObject());

        modulator->addProperty("dsid", dsMeter->getDSID().toString());
        modulator->addProperty("value", isEnergy ? (dsMeter->getCachedEnergyMeterValue() / energyQuotient) : dsMeter->getCachedPowerConsumption());

        DateTime temp_date = isEnergy ? dsMeter->getCachedEnergyMeterTimeStamp() : dsMeter->getCachedPowerConsumptionTimeStamp();
        modulator->addProperty("date", temp_date.toString());

        modulators->addElement("", modulator);
      } catch (std::runtime_error&) {
        return failure("Could not apply properties to JSON object.");
      }
    }
    return success(resultObj);
  }

  WebServerResponse MeteringRequestHandler::jsonHandleRequest(const RestfulRequest& _request, boost::shared_ptr<Session> _session) {
    if(_request.getMethod() == "getResolutions") {
      return getResolutions();
    } else if(_request.getMethod() == "getSeries") {
      return getSeries();
    } else if(_request.getMethod() == "getValues") { //?dsid=;n=,resolution=,type=
      return getValues(_request);
    } else if(_request.getMethod() == "getLatest") {
      return getLatest(_request);
    } else if(_request.getMethod() == "getAggregatedValues") { //?set=;n=,resolution=;type=
      // TODO: implement
    }
    throw std::runtime_error("Unhandled function");
  } // handleRequest

} // namespace dss
