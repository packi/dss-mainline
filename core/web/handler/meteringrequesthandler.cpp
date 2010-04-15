/*
    Copyright (c) 2010 digitalSTROM.org, Zurich, Switzerland

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

#include "meteringrequesthandler.h"

#include <vector>

#include "core/foreach.h"

#include "core/metering/metering.h"
#include "core/metering/series.h"
#include "core/metering/seriespersistence.h"

#include "core/model/modulator.h"
#include "core/model/apartment.h"


#include "core/web/json.h"

namespace dss {


  //=========================================== MeteringRequestHandler

  MeteringRequestHandler::MeteringRequestHandler(Apartment& _apartment, Metering& _metering)
  : m_Apartment(_apartment),
    m_Metering(_metering)
  { }

  boost::shared_ptr<JSONObject> MeteringRequestHandler::getResolutions() {
    std::vector<MeteringConfigChain*> meteringConfig = m_Metering.getConfig();
    boost::shared_ptr<JSONObject> resultObj(new JSONObject());
    boost::shared_ptr<JSONArrayBase> resolutions(new JSONArrayBase());
    resultObj->addElement("resolutions", resolutions);

    foreach(MeteringConfigChain* pChain, meteringConfig) {
      for(int iConfig = 0; iConfig < pChain->size(); iConfig++) {
        boost::shared_ptr<JSONObject> resolution(new JSONObject());
        resolutions->addElement("", resolution);

        resolution->addProperty("type", pChain->isEnergy() ? "energy" : "consumption");
        resolution->addProperty("unit", pChain->getUnit());
        resolution->addProperty("resolution", pChain->getResolution(iConfig));
      }
    }

    return success(resultObj);
  }

  boost::shared_ptr<JSONObject> MeteringRequestHandler::getSeries() {
    boost::shared_ptr<JSONObject> resultObj(new JSONObject());
    boost::shared_ptr<JSONArrayBase> series(new JSONArrayBase());
    resultObj->addElement("series", series);

    std::vector<DSMeter*>& dsMeters = m_Apartment.getDSMeters();
    foreach(DSMeter* dsMeter, dsMeters) {
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
    std::string errorMessage;
    std::string deviceDSIDString = _request.getParameter("dsid");
    std::string resolutionString = _request.getParameter("resolution");
    std::string typeString = _request.getParameter("type");
    std::string fileSuffix;
    std::string storageLocation;
    std::string seriesPath;
    int resolution;
    bool energy;
    if(!deviceDSIDString.empty()) {
      dsid_t deviceDSID = dsid_t::fromString(deviceDSIDString);
      if(!(deviceDSID == NullDSID)) {
        try {
          m_Apartment.getDSMeterByDSID(deviceDSID);
        } catch(std::runtime_error& e) {
          return failure("Could not find device with dsid '" + deviceDSIDString + "'");
        }
      } else {
      return failure("Could not parse dsid '" + deviceDSIDString + "'");
      }
    } else {
      return failure("Could not parse dsid '" + deviceDSIDString + "'");
    }
    resolution = strToIntDef(resolutionString, -1);
    if(resolution == -1) {
      return failure("Need could not parse resolution '" + resolutionString + "'");
    }
    if(typeString.empty()) {
      return failure("Need a type, 'energy' or 'consumption'");
    } else {
      if(typeString == "consumption") {
        energy = false;
      } else if(typeString == "energy") {
        energy = true;
      } else {
        return failure("Invalide type '" + typeString + "'");
      }
    }
    if(!resolutionString.empty()) {
      std::vector<MeteringConfigChain*> meteringConfig = m_Metering.getConfig();
      storageLocation = m_Metering.getStorageLocation();
      for(unsigned int iConfig = 0; iConfig < meteringConfig.size(); iConfig++) {
        MeteringConfigChain* cConfig = meteringConfig[iConfig];
        for(int jConfig = 0; jConfig < cConfig->size(); jConfig++) {
          if(cConfig->isEnergy() == energy && cConfig->getResolution(jConfig) == resolution) {
            fileSuffix = cConfig->getFilenameSuffix(jConfig);
          }
        }
      }
      if(fileSuffix.empty()) {
        return failure("No data for '" + typeString + "' and resolution '" + resolutionString + "'");
      } else {
        seriesPath = storageLocation + deviceDSIDString + "_" + fileSuffix + ".xml";
        log("_Trying to load series from " + seriesPath);
        SeriesReader<CurrentValue> reader;
        boost::shared_ptr<Series<CurrentValue> > s = boost::shared_ptr<Series<CurrentValue> >(reader.readFromXML(seriesPath));
        if(s == NULL) {
          boost::shared_ptr<std::deque<CurrentValue> > values = s->getExpandedValues();

          boost::shared_ptr<JSONObject> resultObj(new JSONObject());
          resultObj->addProperty("dsimid", deviceDSIDString);
          resultObj->addProperty("type", typeString);
          resultObj->addProperty("resolution", resolutionString);
          boost::shared_ptr<JSONArrayBase> valuesArray(new JSONArrayBase);
          resultObj->addElement("values", valuesArray);
          for(std::deque<CurrentValue>::iterator iValue = values->begin(), e = values->end(); iValue != e; iValue++)
          {
            boost::shared_ptr<JSONArrayBase> valuePair(new JSONArrayBase());
            valuesArray->addElement("", valuePair);
            boost::shared_ptr<JSONValue<int> > timeVal(new JSONValue<int>(iValue->getTimeStamp().secondsSinceEpoch()));
            boost::shared_ptr<JSONValue<double> > valueVal(new JSONValue<double>(iValue->getValue()));
            valuePair->addElement("", timeVal);
            valuePair->addElement("", valueVal);
          }

          return success(resultObj);
        } else {
          return failure("No data-file for '" + typeString + "' and resolution '" + resolutionString + "'");
        }
      }
    } else {
      return failure("Could not parse resolution '" + resolutionString + "'");
    }
  }

  boost::shared_ptr<JSONObject> MeteringRequestHandler::getLatest(const RestfulRequest& _request) {
    std::string from = _request.getParameter("from");
    std::string type = _request.getParameter("type");

    if(type.empty() || ((type != "consumption") && (type != "energy"))) {
      return failure("Invalid or missing type parameter");
    }

    if(from.empty()) {
      return failure("Missing 'from' parameter");
    }

    if(from.find(".meters(") == 0) {
      from = from.substr(8);
    } else {
      return failure("Invalid 'from' value");
    }

    if((from.length() > 0) && (from.at(from.length()-1) == ')')) {
      from = from.substr(0, from.length()-1);
    } else {
      return failure("Invalid 'from' value");
    }

    std::vector<DSMeter*> meters;

    if(from == "all") {
      meters = m_Apartment.getDSMeters();
    } else {
      std::vector<std::string> dsids = dss::splitString(from, ',', true);
      for(size_t i = 0; i < dsids.size(); i++) {
        std::string strId = dsids.at(i);
        if(strId.empty()) {
          return failure("Invalid dsid in 'from' value: " + strId);
        }

        dsid_t dsid;
        try {
          dsid = dsid_t::fromString(strId);
        } catch(std::invalid_argument&) {
          return failure("Invalid dsid in 'from' value: " + strId);
        }

        try {
          DSMeter& dsMeter = m_Apartment.getDSMeterByDSID(dsid);
          meters.push_back(&dsMeter);
        } catch (std::runtime_error&) {
          return failure("Could not find dsMeter with given dsid.");
        }

      }//for
    }

    boost::shared_ptr<JSONObject> resultObj(new JSONObject());
    boost::shared_ptr<JSONArrayBase> modulators(new JSONArrayBase());
    resultObj->addElement("values", modulators);

    bool isEnergy = (type == "energy");

    for(size_t i = 0; i < meters.size(); i++) {
      try {
        DSMeter* dsMeter = meters.at(i);
        boost::shared_ptr<JSONObject> modulator(new JSONObject());

        modulator->addProperty("dsid", dsMeter->getDSID().toString());
        modulator->addProperty("value", isEnergy ? dsMeter->getCachedEnergyMeterValue() : dsMeter->getCachedPowerConsumption());

        modulator->addProperty("date", isEnergy ? dsMeter->getCachedEnergyMeterTimeStamp().toString() : dsMeter->getCachedPowerConsumptionTimeStamp().toString());

        modulators->addElement("", modulator);
      } catch (std::runtime_error&) {
        return failure("Could not apply properties to JSON object.");
      }
    }
    return success(resultObj);
  }

  boost::shared_ptr<JSONObject> MeteringRequestHandler::jsonHandleRequest(const RestfulRequest& _request, boost::shared_ptr<Session> _session) {
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
