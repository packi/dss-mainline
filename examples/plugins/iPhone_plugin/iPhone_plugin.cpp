#include <stdlib.h>

#include <sstream>
#include <iomanip>

#include "core/web/plugin/webserver_plugin.h" 
#include "core/base.h"
#include "core/datetools.h" 
#include "core/dss.h" 
#include "core/ds485client.h" 
#include "unix/ds485.h" 
#include "core/ds485const.h" 

using namespace dss;

int plugin_getversion() {
	return WEBSERVER_PLUGIN_API_VERSION;
} // plugin_getversion


bool dataRequest(DSS& _dss, HashMapConstStringString& _parameter,
		std::string& result) {
	std::stringstream sStream;
	std::vector<DSMeter*>& dsMeters = _dss.getApartment().getDSMeters();
	std::vector<Zone*>& zones = _dss.getApartment().getZones();
	
	sStream << "{\"Name\":\""<<_dss.getApartment().getName() <<"\",";
	sStream << "\"Zone\":[";

	bool firstZone = true;
	for (std::vector<Zone*>::iterator ipZone = zones.begin(),
			endZone = zones.end(); ipZone != endZone; ++ipZone) {
		if (firstZone) {
			firstZone = false;
		} else {
			sStream << ",";
		}
		sStream << "{\"Name\":\""<< (*ipZone)->getName() << "\",";
		sStream << "\"ID\":"<< (*ipZone)->getID() << ",";
		sStream << "\"Groups\":[";

		bool firstGroup = true;
		std::vector<Group*> groups = (*ipZone)->getGroups();
		for (std::vector<Group*>::iterator
				ipGroup = groups.begin(), endGroup= groups.end(); ipGroup
				!= endGroup; ++ipGroup) {
			if (firstGroup) {
				firstGroup = false;
			} else {
				sStream << ",";
			}
			sStream << "{\"lastScene\":"<< (*ipGroup)->getLastCalledScene() << ",";
			
			Set pSet=(*ipGroup)->getDevices();
			sStream << "\"DeviceCount\":" << pSet.length() << ",";
			//sStream << "\"DeviceCount\":1,";
			sStream << "\"Name\":\"" << (*ipGroup)->getName() << "\",";

			sStream << "\"ID\":"<< (*ipGroup)->getID() << "}";

		}
		sStream << "]}";
	}

	sStream << "],\"Circuit\":[ ";

	bool firstDSMeter = true;
	for (std::vector<DSMeter*>::iterator ipDSMeter =
			dsMeters.begin(), endDSMeter = dsMeters.end(); ipDSMeter
			!= endDSMeter; ++ipDSMeter) {
		if (firstDSMeter) {
			firstDSMeter = false;
		} else {
			sStream << ",";
		}

		sStream << "{\"Name\":\""<<(*ipDSMeter)->getName() <<"\",";
		sStream << "\"dSID\":\"" << (*ipDSMeter)->getDSID().toString() << "\",";
		sStream << "\"HardwareVersion\":\"" << (*ipDSMeter)->getHardwareVersion() << "\",";
		sStream << "\"SoftwareVersion\":\"" << (*ipDSMeter)->getSoftwareVersion() << "\",";
		sStream << "\"orangeEnergyLevel\":" << (*ipDSMeter)->getEnergyLevelOrange() << ",";
		sStream << "\"redEnergyLevel\":" << (*ipDSMeter)->getEnergyLevelRed() << "";
		sStream << "}";
	}
	sStream << "]}";

	result = sStream.str();
	return true;
}

bool steadyCallRequest(DSS& _dss, HashMapConstStringString& _parameter,
		std::string& result) {
	bool energyAnfrage=strToIntDef(_parameter["parameter"], -1) & 0x01;
	bool lastSceneAnfrage=strToIntDef(_parameter["parameter"], -1) & 0x02;
	bool firstDSMeter;
	DS485Client oClient; 
	DS485CommandFrame frame;

    std::stringstream sStream;
	sStream << "{";
	if (energyAnfrage) 
	{
		frame.getHeader().setBroadcast(1);
	    frame.getHeader().setDestination(0);
	    frame.getHeader().setCounter(1);
	    frame.setCommand(0x09); 
	    frame.getPayload().add<uint8_t>(0x94);
	    oClient.sendFrameDiscardResult(frame);  
		
		
		std::vector<DSMeter*>& dsMeters = _dss.getApartment().getDSMeters();

		sStream << "\"Energy\":[";

		firstDSMeter = true;
		for (std::vector<DSMeter*>::iterator ipDSMeter =
				dsMeters.begin(), endDSMeter = dsMeters.end(); ipDSMeter
				!= endDSMeter; ++ipDSMeter) {
			if (firstDSMeter) {
				firstDSMeter = false;
			} else {
				sStream << ",";
			}

			sStream << "{\"dSID\":\""<< (*ipDSMeter)->getDSID().toString() <<"\",";
			sStream << "\"value\":" << (*ipDSMeter)->getCachedPowerConsumption() << "}";
		}
		
		sStream << "]";
	}
	if (energyAnfrage && lastSceneAnfrage) 
	{
		sStream << ",";
	}
	if (lastSceneAnfrage) 
	{
		sStream << "\"lastScene\":[";
		
		std::vector<Zone*>& zones = _dss.getApartment().getZones();		
		bool firstZone = true;
		for (std::vector<Zone*>::iterator ipZone = zones.begin(),
				endZone = zones.end(); ipZone != endZone; ++ipZone) {
			if (firstZone) {
				firstZone = false;
			} else {
				sStream << ",";
			}
			sStream << "{\"ID\":"<< (*ipZone)->getID() << ",";
			sStream << "\"Groups\":[";

			bool firstGroup = true;
			std::vector<Group*> groups = (*ipZone)->getGroups();
			for (std::vector<Group*>::iterator
					ipGroup = groups.begin(), endGroup= groups.end(); ipGroup
					!= endGroup; ++ipGroup) {
				if (firstGroup) {
					firstGroup = false;
				} else {
					sStream << ",";
				}
				sStream << "{\"lastScene\":"<< (*ipGroup)->getLastCalledScene() << ",";
				sStream << "\"ID\":"<< (*ipGroup)->getID() << "}";

			}
			sStream << "]}";
		}
		
		sStream << "]";
	}
	sStream << "}";
	result = sStream.str();
	
	return true;
}

bool actionRequest(DSS& _dss, HashMapConstStringString& _parameter,
		std::string& result) {
	int iZoneID=strToIntDef(_parameter["zoneID"], -1);
	int iGroupID=strToIntDef(_parameter["groupID"], -1);
	int iSzeneID=strToIntDef(_parameter["szeneID"], -1);
	Zone &pZone=_dss.getApartment().getZone(iZoneID);
	Group *pGroup=pZone.getGroup(iGroupID);
	pGroup->callScene(iSzeneID);
	result = "";
	return true;	
}

bool dimmUpRequest(DSS& _dss, HashMapConstStringString& _parameter,
		std::string& result) {
	int iZoneID=strToIntDef(_parameter["zoneID"], -1);
	int iGroupID=strToIntDef(_parameter["groupID"], -1);
	Zone &pZone=_dss.getApartment().getZone(iZoneID);
	Group *pGroup=pZone.getGroup(iGroupID);
	pGroup->increaseValue();
	result = "";
	return true;	
}

bool dimmDownRequest(DSS& _dss, HashMapConstStringString& _parameter,
		std::string& result) {
	int iZoneID=strToIntDef(_parameter["zoneID"], -1);
	int iGroupID=strToIntDef(_parameter["groupID"], -1);
	Zone &pZone=_dss.getApartment().getZone(iZoneID);
	Group *pGroup=pZone.getGroup(iGroupID);
	pGroup->decreaseValue();
	result = "";
	return true;	
}

bool plugin_handlerequest(const std::string& _uri,
		HashMapConstStringString& _parameter, DSS& _dss,
		std::string& result) {

	if (_uri == "/iPhone/data") {
		return dataRequest(_dss, _parameter, result);
	}
	if (_uri == "/iPhone/steadyCall") {
		return steadyCallRequest(_dss, _parameter, result);
	}
	if (_uri == "/iPhone/action") {
		return actionRequest(_dss, _parameter, result);
	}
	if (_uri == "/iPhone/dimmUp") {
		return dimmUpRequest(_dss, _parameter, result);
	}
	if (_uri == "/iPhone/dimmDown") {
		return dimmDownRequest(_dss, _parameter, result);
	}
	return false;
} // plugin_handlerequest

