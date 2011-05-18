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

#ifndef MODELMAINTENANCE_H_
#define MODELMAINTENANCE_H_

#include <boost/shared_ptr.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/filesystem.hpp>

#include "core/subsystem.h"
#include "core/thread.h"
#include "core/syncevent.h"
#include "core/model/modelevent.h"

namespace dss {
  class Apartment;
  class Event;
  class Metering;
  class StructureQueryBusInterface;

  class ModelMaintenance : public ThreadedSubsystem {
  public:
    ModelMaintenance(DSS* _pDSS, const int _eventTimeoutMS = 1000);
    virtual ~ModelMaintenance() {}

    virtual void initialize();

    /** Adds a model event to the queue.
     * The ownership of the event will reside with the Apartment. ModelEvents arriving while initializing will be discarded.
     */
    void addModelEvent(ModelEvent* _pEvent);

    /** Starts the event-processing */
    virtual void execute();

    void onGroupCallScene(const int _zoneID, const int _groupID, const int _sceneID);
    void onDeviceNameChanged(dss_dsid_t _meterID, const devid_t _deviceID, 
                             const std::string& _name);
    void onDsmNameChanged(dss_dsid_t _meterID, const std::string& _name);

    bool isInitializing() const { return m_IsInitializing; }
    void setApartment(Apartment* _value);
    void setMetering(Metering* _value);
    void setStructureQueryBusInterface(StructureQueryBusInterface* _value);
  protected:
    virtual void doStart();
  private:
    bool handleModelEvents();
    void eraseModelEventsFromQueue(ModelEvent::EventType _type);
    void dsMeterReady(const dss_dsid_t& _dsMeterBusID);
    void discoverDS485Devices();
    void readOutPendingMeter();

    void readConfiguration();
    void writeConfiguration();

    void raiseEvent(boost::shared_ptr<Event> _pEvent);

    void onDeviceCallScene(const dss_dsid_t& _dsMeterID, const int _deviceID, const int _sceneID);
    void onAddDevice(const dss::dss_dsid_t& _dsMeterID, const int _zoneID, const int _devID);
    void onRemoveDevice(const dss_dsid_t& _dsMeterID, const int _zoneID, const int _devID);
    void onLostDSMeter(const dss_dsid_t& _dsMeterID);
    void onDeviceConfigChanged(const dss_dsid_t& _dsMeterID, int _deviceID, 
                               int _configClass, int _configIndex, int _value);
    void rescanDevice(const dss_dsid_t& _dsMeterID, const int _deviceID);
  private:
    bool m_IsInitializing;

    boost::ptr_vector<ModelEvent> m_ModelEvents;
    Mutex m_ModelEventsMutex;
    SyncEvent m_NewModelEvent;
    Apartment* m_pApartment;
    Metering* m_pMetering;
    const int m_EventTimeoutMS;
    StructureQueryBusInterface* m_pStructureQueryBusInterface;

    void checkConfigFile(boost::filesystem::path _filename);
    void readSceneNames();
  }; // ModelMaintenance

}

#endif /* MODELMAINTENANCE_H_ */
