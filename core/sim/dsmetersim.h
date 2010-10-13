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

#ifndef DSMETERSIM_H_
#define DSMETERSIM_H_

#include <map>
#include <vector>

#include "core/ds485types.h"
#include "core/logger.h"

namespace Poco {
  namespace XML {
    class Node;
  }
}

namespace dss {
  class DSSim;
  class DSIDInterface;

  typedef std::map< const std::pair<const int, const int>,  std::vector<DSIDInterface*> > IntPairToDSIDSimVector;

  class DSMeterSim {
  private:
    DSSim* m_pSimulation;
    int m_EnergyLevelOrange;
    int m_EnergyLevelRed;
    int m_ID;
    dss_dsid_t m_DSMeterDSID;
    std::vector<DSIDInterface*> m_SimulatedDevices;
    std::map< const int, std::vector<DSIDInterface*> > m_Zones;
    IntPairToDSIDSimVector m_DevicesOfGroupInZone;
    std::map<const DSIDInterface*, int> m_ButtonToGroupMapping;
    std::map<const DSIDInterface*, int> m_DeviceZoneMapping;
    std::map<const int, std::vector<int> > m_GroupsPerDevice;
    std::map<const int, std::string> m_DeviceNames;
    std::map< const std::pair<const int, const int>, int> m_LastCalledSceneForZoneAndGroup;
    std::string m_Name;
  private:
    void loadDevices(Poco::XML::Node* _node, const int _zoneID);
    void loadGroups(Poco::XML::Node* _node, const int _zoneID);
    void loadZones(Poco::XML::Node* _node);

    DSIDInterface& lookupDevice(const devid_t _id);

    // TODO: libdsm
    // boost::shared_ptr<DS485CommandFrame> createResponse(DS485CommandFrame& _request, uint8_t _functionID) const;
    // boost::shared_ptr<DS485CommandFrame> createAck(DS485CommandFrame& _request, uint8_t _functionID) const;
    // boost::shared_ptr<DS485CommandFrame> createReply(DS485CommandFrame& _request) const;
    //
    // void distributeFrame(boost::shared_ptr<DS485CommandFrame> _frame) const;
    // void sendDelayedResponse(boost::shared_ptr<DS485CommandFrame> _response, int _delayMS);
  private:
    void deviceCallScene(const int _deviceID, const int _sceneID);
    void groupCallScene(const int _zoneID, const int _groupID, const int _sceneID);
    void deviceSaveScene(const int _deviceID, const int _sceneID);
    void groupSaveScene(const int _zoneID, const int _groupID, const int _sceneID);
    void deviceUndoScene(const int _deviceID, const int _sceneID);
    void groupUndoScene(const int _zoneID, const int _groupID, const int _sceneID);
    void groupSetValue(const int _zoneID, const int _groupID, const int _value);
  protected:
    virtual void doStart() {}
    void log(const std::string& _message, aLogSeverity _severity = lsDebug);
  public:
    DSMeterSim(DSSim* _pSimulator);
    virtual ~DSMeterSim() {}

    bool initializeFromNode(Poco::XML::Node* _node);

    int getID() const;
    void setID(const int _value) { m_ID = _value; }

    void setDSID(const dss_dsid_t& _value) { m_DSMeterDSID = _value; }

    DSIDInterface* getSimulatedDevice(const dss_dsid_t _dsid);
    void addSimulatedDevice(DSIDInterface* _device);

    void addDeviceToGroup(DSIDInterface* _device, int _groupID);
  }; // DSMeterSim

}

#endif
