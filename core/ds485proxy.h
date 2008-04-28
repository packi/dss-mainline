/*
 *  ds485proxy.h
 *  dSS
 *
 *  Created by Patrick Stählin on 4/18/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef _DS485_PROXY_H_INCLUDED
#define _DS485_PROXY_H_INCLUDED

#include "model.h"

namespace dss {
  
  typedef hash_map<const Modulator*, pair< vector<Group*>, Set> > FittingResult;
  
  
  template<class t>
  class DS485Parameter {
  private:
    t _data;
  };

  
  class DS485Header {
  private:
    unsigned int m_Destination;
    bool m_Broadcast;
    unsigned int m_Source;
    unsigned int m_Counter;
    int m_Type;
  };
  
  class DS485Frame {
  private:
    DS485Header m_Header;
  public:
  }; // DS485Frame
  
  class DS485CommandFrame {
  private:
    //vector<DS485Parameter*> m_Parameters;
    unsigned short m_Command;
    unsigned short m_Length;
  };
  
  typedef enum {
    cmdTurnOn,
    cmdTurnOff,
    cmdStartDimUp,
    cmdStartDimDown,
    cmdStopDim,
    cmdCallScene,
    cmdSaveScene,
    cmdUndoScene,
    cmdIncreaseValue,
    cmdDecreaseValue,
    cmdEnable,
    cmdDisable,
    cmdIncreaseParam,
    cmdDecreaseParam
  } DS485Command;
  
  class DS485Interface {
  public:
    virtual void Send(DS485Frame& _frame) = 0;
    virtual DS485Frame& Receive() = 0;
  };
  
  //================================================== Simulation stuff ahead
  
  class DSIDSim;
  
  class DSModulatorSim : public DS485Interface {
  private:
    Apartment& m_Apartment;
    vector<DSIDSim*> m_SimulatedDevices;
  public:
    DSModulatorSim(Apartment& _apartment, const string& _configFile);

    virtual void Send(DS485Frame& _frame) ;
    virtual DS485Frame& Receive();
  };
  
  class DSIDSim : public IDeviceInterface {
  private:
    devid_t m_DSId;
    bool m_On;
    bool m_Enabled;
    vector<int> m_Parameters;
    DSModulatorSim* m_Modulator;
  public:
    DSIDSim(const devid_t _DSId);
    
    virtual void TurnOn();
    virtual void TurnOff();
    
    virtual void IncreaseValue(const int _parameterNr = -1);
    virtual void DecreaseValue(const int _parameterNr = -1);
    
    virtual void Enable();
    virtual void Disable();
    
    virtual void StartDim(bool _directionUp, const int _parameterNr = -1);
    virtual void EndDim(const int _parameterNr = -1);
    virtual void SetValue(const double _value, int _parameterNr = -1);
  };
  
  class DS485Proxy {
  private:
    FittingResult BestFit(Set& _set);
    bool IsSimAddress(devid_t _addr);
  public:
    //------------------------------------------------ Specialized Commands (system)
    vector<int> GetModulators();
    vector<int> GetDevices(const int _modulatorID);
    int GetGroupCount(const int _modulatorID);
    vector<int> GetDevicesInGroup(const int _modulatorID, const int _modulatorID);
    
    vector<int> GetRooms(const int _modulatorID);
    int GetRoomCount(const int _modulatorID);
    vector<int> GetDevicesInRoom(const int _modulatorID, const int _roomID);
    
    void AddToGroup(const int _modulatorID, const int _groupID, const int _deviceID);
    void RemoveFromGroup(const int _modulatorID, const int _groupID, const int _deviceID);
    
    int AddUserGroup(const int _modulatorID);
    void RemoveUserGroup(const int _modulatorID, const int _groupID);
    
    //------------------------------------------------ Device manipulation    
    void SendCommand(DS485Command _cmd, Set& _set);
    void SendCommand(DS485Command _cmd, Device& _device);
    void SendCommand(DS485Command _cmd, devid_t _id);
  };
}

#endif
