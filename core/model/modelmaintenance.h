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

#include "core/subsystem.h"
#include "core/thread.h"
#include "core/syncevent.h"

namespace dss {
  class PropertyNode;
  typedef boost::shared_ptr<PropertyNode> PropertyNodePtr;
  class Apartment;
  class ModelEvent;
  class Event;


  class ModelMaintenance : public Subsystem,
                           public Thread {
  public:
    ModelMaintenance(DSS* _pDSS);
    virtual ~ModelMaintenance() {}

    virtual void initialize();

    /** Adds a model event to the queue.
     * The ownership of the event will reside with the Apartment. ModelEvents arriving while initializing will be discarded.
     */
    void addModelEvent(ModelEvent* _pEvent);

    /** Starts the event-processing */
    virtual void execute();

    void onGroupCallScene(const int _zoneID, const int _groupID, const int _sceneID);

    bool isInitializing() const { return m_IsInitializing; }
    void setApartment(Apartment* _value);
  protected:
    virtual void doStart();
  private:
    void handleModelEvents();
    void dsMeterReady(int _dsMeterBusID);
    void discoverDS485Devices();
    void waitForInterface();

    void readConfiguration();
    void writeConfiguration();

    void raiseEvent(boost::shared_ptr<Event> _pEvent);

    void onDeviceCallScene(const int _dsMeterID, const int _deviceID, const int _sceneID);
    void onAddDevice(const int _modID, const int _zoneID, const int _devID, const int _functionID);
    void onDSLinkInterrupt(const int _modID, const int _devID, const int _priority);
  private:
    bool m_IsInitializing;

    PropertyNodePtr m_pPropertyNode;

    boost::ptr_vector<ModelEvent> m_ModelEvents;
    Mutex m_ModelEventsMutex;
    SyncEvent m_NewModelEvent;
    Apartment* m_pApartment;
  }; // ModelMaintenance

}

#endif /* MODELMAINTENANCE_H_ */
