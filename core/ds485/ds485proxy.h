/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland

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

#ifndef _DS485_PROXY_H_INCLUDED
#define _DS485_PROXY_H_INCLUDED

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "core/ds485types.h"
#include "ds485.h"
#include "core/syncevent.h"
#include "core/DS485Interface.h"
#include "core/subsystem.h"
#include "core/mutex.h"

#include <map>
#include <vector>

#ifndef WIN32
  #include <ext/hash_map>
#else
  #include <hash_map>
#endif

#ifndef WIN32
using namespace __gnu_cxx;
#else
using namespace stdext;
#endif

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/shared_ptr.hpp>

namespace dss {

  class DS485Proxy;
  class Apartment;
  class ModelEvent;

  /** A ReceivedFrame stores a boost::shared_ptr to the frame as well as the token-counter
   *  of its arrival.
   */
  class ReceivedFrame {
  private:
    int m_ReceivedAtToken;
    boost::shared_ptr<DS485CommandFrame> m_Frame;
  public:
    ReceivedFrame(const int _receivedAt, boost::shared_ptr<DS485CommandFrame> _frame);
    boost::shared_ptr<DS485CommandFrame> getFrame() { return m_Frame; };

    /** Returns the arrival time in (owned) tokens */
    int getReceivedAt() const { return m_ReceivedAtToken; };
  }; // ReceivedFrame


  /** A frame bucket gets notified on every frame that matches any given
   *  function-/source-id pair.
   *  If \a m_SourceID is -1 every source matches. */
  class FrameBucketBase {
  public:
    FrameBucketBase(DS485Proxy* _proxy, int _functionID, int _sourceID);
    virtual ~FrameBucketBase() {}

    int getFunctionID() const { return m_FunctionID; }
    int getSourceID() const { return m_SourceID; }

    virtual bool addFrame(boost::shared_ptr<ReceivedFrame> _frame) = 0;

    /** Registers the bucket at m_pProxy */
    void addToProxy();
    /** Removes the bucket from m_pProxy */
    void removeFromProxy();
    /** Static function to be used from a boost::shared_ptr as a deleter. */
    static void removeFromProxyAndDelete(FrameBucketBase* _obj);
  private:
    DS485Proxy* m_pProxy;
    int m_FunctionID;
    int m_SourceID;
  }; // FrameBucketBase


  /** FrameBucketCollector holds its received frames in a queue.
    */
  class FrameBucketCollector : public FrameBucketBase {
  private:
    std::deque<boost::shared_ptr<ReceivedFrame> > m_Frames;
    SyncEvent m_PacketHere;
    Mutex m_FramesMutex;
    bool m_SingleFrame;
  public:
    FrameBucketCollector(DS485Proxy* _proxy, int _functionID, int _sourceID);
    virtual ~FrameBucketCollector() { }

    /** Adds a ReceivedFrame to the frames queue */
    virtual bool addFrame(boost::shared_ptr<ReceivedFrame> _frame);
    /** Returns the least recently received item int the queue.
     * The pointer will contain NULL if isEmpty() returns true. */
    boost::shared_ptr<ReceivedFrame> popFrame();

    /** Waits for frames to arrive for \a _timeoutMS */
    void waitForFrames(int _timeoutMS);
    /** Waits for a frame to arrive in \a _timeoutMS.
     * If a frame arrives earlier, the function returns */
    bool waitForFrame(int _timeoutMS);

    int getFrameCount() const;
    bool isEmpty() const;
  }; // FrameBucketCollector


  typedef std::vector<boost::shared_ptr<DS485CommandFrame> > CommandFrameSharedPtrVector;

  class DS485Proxy : public    Thread,
                     public    Subsystem,
                     public    DS485Interface,
                     public    DeviceBusInterface,
                     public    StructureQueryBusInterface,
                     public    MeteringBusInterface,
                     public    StructureModifyingBusInterface,
                     public    FrameSenderInterface,
                     public    IDS485FrameCollector {
  private:
#ifdef WITH_SIM
    bool isSimAddress(const uint8_t _addr);
#endif

    /** Returns a single frame or NULL if none should arrive within the timeout (1000ms) */
    boost::shared_ptr<ReceivedFrame> receiveSingleFrame(DS485CommandFrame& _frame, uint8_t _functionID);
    uint8_t receiveSingleResult(DS485CommandFrame& _frame, const uint8_t _functionID);
    uint16_t receiveSingleResult16(DS485CommandFrame& _frame, const uint8_t _functionID);

    std::vector<FrameBucketBase*> m_FrameBuckets;

    void signalEvent();

    DS485Controller m_DS485Controller;
    SyncEvent m_ProxyEvent;
    Apartment* m_pApartment;

    SyncEvent m_PacketHere;
    Mutex m_IncomingFramesGuard;
    Mutex m_FrameBucketsGuard;
    CommandFrameSharedPtrVector m_IncomingFrames;
    bool m_InitializeDS485Controller;

    DSMeterSpec_t dsMeterSpecFromFrame(boost::shared_ptr<DS485CommandFrame> _frame);
    void checkResultCode(const int _resultCode);
    void raiseModelEvent(ModelEvent* _pEvent);
  protected:
    virtual void doStart();
  public:
    DS485Proxy(DSS* _pDSS, Apartment* _pApartment);
    virtual ~DS485Proxy() {};

    virtual DeviceBusInterface* getDeviceBusInterface() { return this; }
    virtual StructureQueryBusInterface* getStructureQueryBusInterface() { return this; }
    virtual MeteringBusInterface* getMeteringBusInterface() { return this; }
    virtual StructureModifyingBusInterface* getStructureModifyingBusInterface() { return this; }
    virtual FrameSenderInterface* getFrameSenderInterface() { return this; }

    virtual bool isReady();
    void setInitializeDS485Controller(const bool _value) { m_InitializeDS485Controller = _value; }
    virtual void execute();

    virtual void sendFrame(DS485CommandFrame& _frame);
    boost::shared_ptr<FrameBucketCollector> sendFrameAndInstallBucket(DS485CommandFrame& _frame, const int _functionID);
    void installBucket(boost::shared_ptr<FrameBucketBase> _bucket);

    //------------------------------------------------ Handling
    virtual void initialize();
    void waitForProxyEvent();

    virtual void collectFrame(boost::shared_ptr<DS485CommandFrame> _frame);

    void addFrameBucket(FrameBucketBase* _bucket);
    void removeFrameBucket(FrameBucketBase* _bucket);

    //------------------------------------------------ Specialized Commands (system)
    virtual std::vector<DSMeterSpec_t> getDSMeters();
    virtual DSMeterSpec_t getDSMeterSpec(const int _dsMeterID);

    virtual std::vector<int> getZones(const int _dsMeterID);
    virtual int getZoneCount(const int _dsMeterID);
    virtual std::vector<int> getDevicesInZone(const int _dsMeterID, const int _zoneID);
    virtual int getDevicesCountInZone(const int _dsMeterID, const int _zoneID);

    virtual void setZoneID(const int _dsMeterID, const devid_t _deviceID, const int _zoneID);
    virtual void createZone(const int _dsMeterID, const int _zoneID);
    virtual void removeZone(const int _dsMeterID, const int _zoneID);

    virtual int getGroupCount(const int _dsMeterID, const int _zoneID);
    virtual std::vector<int> getGroups(const int _dsMeterID, const int _zoneID);
    virtual int getDevicesInGroupCount(const int _dsMeterID, const int _zoneID, const int _groupID);
    virtual std::vector<int> getDevicesInGroup(const int _dsMeterID, const int _zoneID, const int _groupID);

    virtual std::vector<int> getGroupsOfDevice(const int _dsMeterID, const int _deviceID);

    virtual void addToGroup(const int _dsMeterID, const int _groupID, const int _deviceID);
    virtual void removeFromGroup(const int _dsMeterID, const int _groupID, const int _deviceID);

    virtual int addUserGroup(const int _dsMeterID);
    virtual void removeUserGroup(const int _dsMeterID, const int _groupID);

    virtual dsid_t getDSIDOfDevice(const int _dsMeterID, const int _deviceID);
    virtual dsid_t getDSIDOfDSMeter(const int _dsMeterID);

    virtual int getLastCalledScene(const int _dsMeterID, const int _zoneID, const int _groupID);

    virtual unsigned long getPowerConsumption(const int _dsMeterID);
    virtual unsigned long getEnergyMeterValue(const int _dsMeterID);
    virtual bool getEnergyBorder(const int _dsMeterID, int& _lower, int& _upper);

    //------------------------------------------------ UDI
    virtual uint8_t dSLinkSend(const int _dsMeterID, devid_t _devAdr, uint8_t _value, uint8_t _flags);

    //------------------------------------------------ Device
    virtual uint16_t deviceGetParameterValue(devid_t _id, uint8_t _dsMeterID, int _paramID);
    virtual uint16_t deviceGetFunctionID(devid_t _id, uint8_t _dsMeterID);

    void setValueDevice(const Device& _device, const uint16_t _value, const uint16_t _parameterID, const int _size);
    virtual int getSensorValue(const Device& _device, const int _sensorID);
    //------------------------------------------------ Helpers
    DS485Controller& getController() { return m_DS485Controller; }
  }; // DS485Proxy

} // namespace dss

#endif
