/*
 * DS485Interface.h
 *
 *  Created on: Aug 12, 2008
 *      Author: Patrick Stählin
 */

#ifndef DS485INTERFACE_H_
#define DS485INTERFACE_H_

#include "ds485types.h"
#include "model.h"

#include <vector>

using namespace std;

namespace dss {


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
	  cmdDecreaseParam,
	  cmdGetOnOff,
	  cmdGetValue
	} DS485Command;

  class DS485Interface {
  public:
	DS485Interface();
	virtual ~DS485Interface();

    //------------------------------------------------ Specialized Commands (system)
    virtual vector<int> GetModulators() = 0;

    virtual vector<int> GetRooms(const int _modulatorID) = 0;
    virtual int GetRoomCount(const int _modulatorID) = 0;
    virtual vector<int> GetDevicesInRoom(const int _modulatorID, const int _roomID) = 0;
    virtual int GetDevicesCountInRoom(const int _modulatorID, const int _roomID) = 0;

    virtual int GetGroupCount(const int _modulatorID, const int _roomID) = 0;
    virtual vector<int> GetGroups(const int _modulatorID, const int _roomID) = 0;
    virtual int GetDevicesInGroupCount(const int _modulatorID, const int _roomID, const int _groupID) = 0;
    virtual vector<int> GetDevicesInGroup(const int _modulatorID, const int _roomID, const int _groupID) = 0;

    virtual void AddToGroup(const int _modulatorID, const int _groupID, const int _deviceID) = 0;
    virtual void RemoveFromGroup(const int _modulatorID, const int _groupID, const int _deviceID) = 0;

    virtual int AddUserGroup(const int _modulatorID) = 0;
    virtual void RemoveUserGroup(const int _modulatorID, const int _groupID) = 0;

    virtual dsid_t GetDSIDOfDevice(const int _modulatorID, const int _deviceID) = 0;
    virtual dsid_t GetDSIDOfModulator(const int _modulatorID) = 0;

    //------------------------------------------------ Device manipulation
    virtual vector<int> SendCommand(DS485Command _cmd, Set& _set) = 0;
    virtual vector<int> SendCommand(DS485Command _cmd, Device& _device) = 0;
    virtual vector<int> SendCommand(DS485Command _cmd, devid_t _id, uint8 _modulatorID) = 0;
    virtual vector<int> SendCommand(DS485Command _cmd, const Modulator& _modulator, Group& _group) = 0;
  };
}
#endif /* DS485INTERFACE_H_ */
