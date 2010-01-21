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

#include "businterfacehandler.h"

#include <sstream>

#include "core/foreach.h"
#include "core/dss.h"
#include "core/model/modelevent.h"
#include "core/model/modelmaintenance.h"
#include "core/ds485const.h"
#include "core/event.h"
#include "core/ds485/framebucketbase.h"
#include "core/sim/dssim.h"

namespace dss {

  const char* FunctionIDToString(const int _functionID);

  //================================================== BusInterfaceHandler

  BusInterfaceHandler::BusInterfaceHandler(DSS* _pDSS, ModelMaintenance& _apartment)
  : Thread("BusInterfaceHandler"),
    Subsystem(_pDSS, "BusInterfaceHandler"),
    m_ModelMaintenance(_apartment)
  {}

  void BusInterfaceHandler::initialize() {
    Subsystem::initialize();
  } // initialize

  void BusInterfaceHandler::execute() {
    while(!m_Terminated) {
      if(!m_IncomingFrames.empty() || m_PacketHere.waitFor(50)) {
        while(!m_IncomingFrames.empty()) {
          m_IncomingFramesGuard.lock();
          // process packets and put them into a functionID-hash
          boost::shared_ptr<DS485CommandFrame> frame = m_IncomingFrames.front();
          m_IncomingFrames.erase(m_IncomingFrames.begin());
          m_IncomingFramesGuard.unlock();
          log("R");

          const std::vector<unsigned char>& ch = frame->getPayload().toChar();
          if(ch.size() < 1) {
            log("received Command Frame w/o function identifier", lsFatal);
            continue;
          }

          uint8_t functionID = ch.front();
          if((frame->getCommand() == CommandRequest || frame->getCommand() == CommandEvent) && functionID != FunctionDSLinkReceive) {
            std::string functionIDStr = FunctionIDToString(functionID);
            if(functionIDStr.empty()) {
              functionIDStr = "Unknown function id: " + intToString(functionID, true);
            }
            std::ostringstream sstream;
            sstream << "Got request: " << functionIDStr << " from " << int(frame->getHeader().getSource()) << " ";
            if(frame->getFrameSource() == fsWire) {
              sstream << "over the wire ";
            } else {
              sstream << "from the dss ";
            }

            PayloadDissector pdDump(frame->getPayload());
            while(!pdDump.isEmpty()) {
              uint8_t data = pdDump.get<uint8_t>();
              sstream << "(0x" << std::hex << (unsigned int)data << ", " << std::dec << (int)data << "d)";
            }
            sstream << std::dec;
            log(sstream.str());


            PayloadDissector pd(frame->getPayload());

#ifdef WITH_SIM
            if(frame->getFrameSource() == fsWire) {
              getDSS().getSimulation().process(*frame.get());
            }
#endif
            if(functionID == FunctionZoneAddDevice) {
              log("New device");
              pd.get<uint8_t>(); // function id
              int modID = frame->getHeader().getSource();
              int zoneID = pd.get<uint16_t>();
              int devID = pd.get<uint16_t>();
              pd.get<uint16_t>(); // version
              int functionID = pd.get<uint16_t>();

              ModelEvent* pEvent = new ModelEvent(ModelEvent::etNewDevice);
              pEvent->addParameter(modID);
              pEvent->addParameter(zoneID);
              pEvent->addParameter(devID);
              pEvent->addParameter(functionID);
              raiseModelEvent(pEvent);
            } else if(functionID == FunctionGroupCallScene) {
              pd.get<uint8_t>(); // function id
              uint16_t zoneID = pd.get<uint16_t>();
              uint16_t groupID = pd.get<uint16_t>();
              uint16_t sceneID = pd.get<uint16_t>();
              if(frame->getCommand() == CommandRequest) {
                boost::shared_ptr<Event> sceneEvent(new Event("callScene"));
                sceneEvent->setProperty("sceneID", intToString(sceneID & 0x00ff));
                sceneEvent->setProperty("groupID", intToString(groupID));
                sceneEvent->setProperty("zoneID", intToString(zoneID));
                if(DSS::hasInstance()) {
                  getDSS().getEventQueue().pushEvent(sceneEvent);
                }
              }
              ModelEvent* pEvent = new ModelEvent(ModelEvent::etCallSceneGroup);
              pEvent->addParameter(zoneID);
              pEvent->addParameter(groupID);
              pEvent->addParameter(sceneID);
              raiseModelEvent(pEvent);
            } else if(functionID == FunctionDeviceCallScene) {
              pd.get<uint8_t>(); // functionID
              uint16_t devID = pd.get<uint16_t>();
              uint16_t sceneID = pd.get<uint16_t>();
              int modID = frame->getHeader().getDestination();
              ModelEvent* pEvent = new ModelEvent(ModelEvent::etCallSceneDevice);
              pEvent->addParameter(modID);
              pEvent->addParameter(devID);
              pEvent->addParameter(sceneID);
              raiseModelEvent(pEvent);
            } else if(functionID == EventDSLinkInterrupt) {
              pd.get<uint8_t>(); // functionID
              uint16_t devID = pd.get<uint16_t>();
              uint16_t priority = pd.get<uint16_t>();
              int modID = frame->getHeader().getSource();
              ModelEvent* pEvent = new ModelEvent(ModelEvent::etDSLinkInterrupt);
              pEvent->addParameter(modID);
              pEvent->addParameter(devID);
              pEvent->addParameter(priority);
              raiseModelEvent(pEvent);
            } else if(functionID == EventDeviceReceivedTelegramShort) {
              pd.get<uint8_t>(); // function id
              uint16_t p1 = pd.get<uint16_t>();
              uint16_t p2 = pd.get<uint16_t>();
              uint16_t p3 = pd.get<uint16_t>();
              uint16_t address = p1 & 0x007F;
              uint16_t buttonNumber = p2 & 0x000F;
              uint16_t kind = p3 & 0x000F;
              boost::shared_ptr<Event> buttonEvt(new Event("buttonPressed"));
              buttonEvt->setProperty("address", intToString(address));
              buttonEvt->setProperty("buttonNumber", intToString(buttonNumber));
              buttonEvt->setProperty("kind", intToString(kind));
              getDSS().getEventQueue().pushEvent(buttonEvt);
            } else if(functionID == EventDeviceReceivedTelegramLong) {
              pd.get<uint8_t>(); // function id
              pd.get<uint16_t>();
              uint16_t p2 = pd.get<uint16_t>();
              uint16_t p3 = pd.get<uint16_t>();
              uint16_t p4 = pd.get<uint16_t>();
              uint16_t address = ((p3&0x0f00) | (p4&0x00f0) | (p4&0x000f))>>2;
              uint16_t subqualifier = ((p4 & 0xf000)>>12);
              uint8_t mainqualifier = (p4&0x0f00)>>8;
              uint16_t data = ((p2 &0x0f00)<< 4)&0xf000;
              data |= ((p3&0x00f0) << 4) &0x0f00;
              data |= ((p3 &0x000f)<<4)&0x00f0;
              data |= ((p3&0xf000)>> 12) &0x000f;
              boost::shared_ptr<Event> telEvt(new Event("deviceReceivedTelegram"));
              telEvt->setProperty("data", intToString(data));
              telEvt->setProperty("address", intToString(address));
              telEvt->setProperty("subqualifier", intToString(subqualifier));
              telEvt->setProperty("mainqualifier", intToString(mainqualifier));
              getDSS().getEventQueue().pushEvent(telEvt);
            } else if(functionID == EventNewDS485Device) {
              pd.get<uint8_t>(); // functionID
              int modID = pd.get<uint16_t>();
              ModelEvent* pEvent = new ModelEvent(ModelEvent::etNewDSMeter);
              pEvent->addParameter(modID);
              raiseModelEvent(pEvent);
            } else if(functionID == EventLostDS485Device) {
              pd.get<uint8_t>(); // functionID
              int modID = pd.get<uint16_t>();
              ModelEvent* pEvent = new ModelEvent(ModelEvent::etLostDSMeter);
              pEvent->addParameter(modID);
              raiseModelEvent(pEvent);
            } else if(functionID == EventDeviceReady) {
              int modID = frame->getHeader().getDestination();
              ModelEvent* pEvent = new ModelEvent(ModelEvent::etDSMeterReady);
              pEvent->addParameter(modID);
              raiseModelEvent(pEvent);
            }
          } else {
            std::ostringstream sstream;
            sstream << "Response: ";
            PayloadDissector pd(frame->getPayload());
            while(!pd.isEmpty()) {
              uint8_t data = pd.get<uint8_t>();
              sstream << "(0x" << std::hex << (unsigned int)data << ", " << std::dec << (int)data << "d)";
            }
            sstream << std::dec;
            sstream << " from " << int(frame->getHeader().getSource());
            log(sstream.str());

            log(std::string("Response for: ") + FunctionIDToString(functionID));

            PayloadDissector pd2(frame->getPayload());
            pd2.get<uint8_t>();
            if (functionID == FunctionDSMeterGetPowerConsumption) {
              int modID = frame->getHeader().getSource();
              ModelEvent* pEvent = new ModelEvent(ModelEvent::etPowerConsumption);
              pEvent->addParameter(modID);
              pEvent->addParameter(pd2.get<uint32_t>());
              raiseModelEvent(pEvent);
            } else if (functionID == FunctionDSMeterGetEnergyMeterValue) {
              int modID = frame->getHeader().getSource();
              ModelEvent* pEvent = new ModelEvent(ModelEvent::etEnergyMeterValue);
              pEvent->addParameter(modID);
              pEvent->addParameter(pd2.get<uint32_t>());
              raiseModelEvent(pEvent);
            } else if (functionID == FunctionDSMeterGetDSID) {
              int sourceID = frame->getHeader().getSource();
              ModelEvent* pEvent = new ModelEvent(ModelEvent::etDS485DeviceDiscovered);
              pEvent->addParameter(sourceID);
              pEvent->addParameter(((pd2.get<uint8_t>() << 8) & 0xff00) | (pd2.get<uint8_t>() & 0x00ff));
              pEvent->addParameter(((pd2.get<uint8_t>() << 8) & 0xff00) | (pd2.get<uint8_t>() & 0x00ff));
              pEvent->addParameter(((pd2.get<uint8_t>() << 8) & 0xff00) | (pd2.get<uint8_t>() & 0x00ff));
              pEvent->addParameter(((pd2.get<uint8_t>() << 8) & 0xff00) | (pd2.get<uint8_t>() & 0x00ff));
              pEvent->addParameter(((pd2.get<uint8_t>() << 8) & 0xff00) | (pd2.get<uint8_t>() & 0x00ff));
              pEvent->addParameter(((pd2.get<uint8_t>() << 8) & 0xff00) | (pd2.get<uint8_t>() & 0x00ff));
              raiseModelEvent(pEvent);
            }

            bool bucketFound = false;
            // search for a bucket to put the frame in
            m_FrameBucketsGuard.lock();
            foreach(FrameBucketBase* bucket, m_FrameBuckets) {
              if(bucket->getFunctionID() == functionID) {
                if((bucket->getSourceID() == -1) || (bucket->getSourceID() == frame->getHeader().getSource())) {
                  if(bucket->addFrame(frame)) {
                    bucketFound = true;
                  }
                }
              }
            }
            m_FrameBucketsGuard.unlock();
            if(!bucketFound) {
              log("No bucket found for " + intToString(frame->getHeader().getSource()));
            }

          }
        }
      }
    }
  } // execute

  void BusInterfaceHandler::raiseModelEvent(ModelEvent* _pEvent) {
    m_ModelMaintenance.addModelEvent(_pEvent);
  } // raiseModelEvent

  void BusInterfaceHandler::collectFrame(boost::shared_ptr<DS485CommandFrame> _frame) {
    uint8_t commandID = _frame->getCommand();
    if(commandID != CommandResponse && commandID != CommandRequest && commandID != CommandEvent) {
      log("discarded non response/request/command frame", lsInfo);
      log(std::string("frame type ") + commandToString(commandID));
    } else {
      m_IncomingFramesGuard.lock();
      m_IncomingFrames.push_back(_frame);
      m_IncomingFramesGuard.unlock();
      m_PacketHere.signal();
    }
  } // collectFrame

  void BusInterfaceHandler::addFrameBucket(FrameBucketBase* _bucket) {
    m_FrameBucketsGuard.lock();
    m_FrameBuckets.push_back(_bucket);
    m_FrameBucketsGuard.unlock();
  } // addFrameBucket

  void BusInterfaceHandler::removeFrameBucket(FrameBucketBase* _bucket) {
    m_FrameBucketsGuard.lock();
    std::vector<FrameBucketBase*>::iterator pos = find(m_FrameBuckets.begin(), m_FrameBuckets.end(), _bucket);
    if(pos != m_FrameBuckets.end()) {
      m_FrameBuckets.erase(pos);
    }
    m_FrameBucketsGuard.unlock();
  } // removeFrameBucket

  void BusInterfaceHandler::doStart() {
    // call Thread::run()
    run();
  } // doStart


  //================================================== Helper

  const char* FunctionIDToString(const int _functionID) {
    switch(_functionID) {
    case  FunctionDSMeterAddZone:
      return "DSMeter Add Zone";
    case  FunctionDSMeterRemoveZone:
      return "DSMeter Remove Zone";
    case  FunctionDSMeterRemoveAllZones:
      return "DSMeter Remove All Zones";
    case  FunctionDSMeterCountDevInZone:
      return "DSMeter Count Dev In Zone";
    case  FunctionDSMeterDevKeyInZone:
      return "DSMeter Dev Key In Zone";
    case  FunctionDSMeterGetGroupsSize:
      return "DSMeter Get Groups Size";
    case  FunctionDSMeterGetZonesSize:
      return "DSMeter Get Zones Size";
    case  FunctionDSMeterGetZoneIdForInd:
      return "DSMeter Get Zone Id For Index";
    case  FunctionDSMeterAddToGroup:
      return "DSMeter Add To Group";
    case  FunctionDSMeterRemoveFromGroup:
      return "DSMeter Remove From Group";
    case  FunctionGroupAddDeviceToGroup:
      return "Group Add Device";
    case  FunctionGroupRemoveDeviceFromGroup:
      return "Group Remove Device";
    case  FunctionGroupGetDeviceCount:
      return "Group Get Device Count";
    case  FunctionGroupGetDevKeyForInd:
      return "Group Get Dev Key For Index";

    case  FunctionZoneGetGroupIdForInd:
      return "Zone Get Group ID For Index";

    case  FunctionDeviceCallScene:
      return "Device Call Scene";
    case  FunctionDeviceSaveScene:
      return "Device Save Scene";
    case  FunctionDeviceUndoScene:
      return "Device Undo Scene";

    case FunctionDeviceIncreaseValue:
      return "Function Device Increase Value";
    case FunctionDeviceDecreaseValue:
      return "Function Device Decrease Value";
    case FunctionDeviceStartDimInc:
      return "Function Device Start Dim Inc";
    case FunctionDeviceStartDimDec:
      return "Function Device Start Dim Dec";
    case FunctionDeviceEndDim:
      return "Function Device End Dim";

    case  FunctionGroupCallScene:
      return "Group Call Scene";
    case  FunctionGroupSaveScene:
      return "Group Save Scene";
    case  FunctionGroupUndoScene:
      return "Group Undo Scene";

    case FunctionGroupIncreaseValue:
      return "Function Group Increase Value";
    case FunctionGroupDecreaseValue:
      return "Function Group Decrease Value";
    case FunctionGroupStartDimInc:
      return "Function Group Start Dim Inc";
    case FunctionGroupStartDimDec:
      return "Function Group Start Dim Dec";
    case FunctionGroupEndDim:
      return "Function Group End Dim";


    case FunctionDeviceSetZoneID:
      return "Device Set ZoneID";

    case  FunctionDeviceGetOnOff:
      return "Function Device Get On Off";
    case  FunctionDeviceGetParameterValue:
      return "Function Device Get Parameter Value";
    case  FunctionDeviceGetDSID:
      return "Function Device Get DSID";
    case FunctionDeviceGetGroups:
      return "Function Device Get Groups";
    case FunctionDeviceGetSensorValue:
      return "Function Device Get Sensor Value";

    case FunctionDSMeterGetDSID:
      return "Function DSMeter Get DSID";

    case FunctionDSMeterGetPowerConsumption:
      return "Function DSMeter Get PowerConsumption";
    case FunctionDSMeterGetEnergyMeterValue:
      return "Function DSMeter Get Energy-Meter Value";
    case FunctionDSMeterGetEnergyLevel:
      return "Function DSMeter Get Energy-Level";
    case FunctionDSMeterSetEnergyLevel:
      return "Function DSMeter Set Energy-Level";

    case FunctionGetTypeRequest:
      return "Function Get Type";

    case FunctionMeterSynchronisation:
      return "Function Meter Synchronization";

    case FunctionDeviceGetFunctionID:
      return "Function Device Get Function ID";
    case FunctionDSLinkConfigWrite:
      return "Function dSLink Config Write";
    case FunctionDSLinkConfigRead:
      return "Function dSLink Config Read";
    case FunctionDSLinkSendDevice:
      return "Function dSLink Send Device";
    case FunctionDSLinkSendGroup:
      return "Function dSLink Send Group";
    case FunctionDSLinkReceive:
      return "Function dSLink Receive";
    case EventDSLinkInterrupt:
      return "Function DSLink Interrupt";


    case FunctionZoneAddDevice:
      return "Function Zone Add Device";
    case FunctionZoneRemoveDevice:
      return "Function Zone Remove Device";
    case FunctionDeviceAddToGroup:
      return "Function Device Add To Group";
    case EventNewDS485Device:
      return "Event New DS485 Device";
    case EventLostDS485Device:
      return "Event Lost DS485 Device";
    case EventDeviceReceivedTelegramShort:
      return "Event Telegram Short";
    case EventDeviceReceivedTelegramLong:
      return "Event Telegram Long";
    case EventDeviceReady:
      return "Event Device Ready";
    }
    return "";
  } // functionIDToString


} // namespace dss
