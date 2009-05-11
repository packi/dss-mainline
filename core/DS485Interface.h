/*
 * DS485Interface.h
 *
 *  Created on: Aug 12, 2008
 *      Author: Patrick St�hlin
 */

#ifndef DS485INTERFACE_H_
#define DS485INTERFACE_H_

#include "ds485types.h"
#include "unix/ds485.h"
#include "model.h"

#include <vector>

using namespace std;

namespace dss {

	/** Commands to be transmitted either to a set, group or a single device. */
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
	  cmdGetValue,
	  cmdSetValue,
	  cmdGetFunctionID
	} DS485Command;

  /** Interface to be implemented by any implementation of the DS485 interface */
  class DS485Interface {
  public:
    virtual ~DS485Interface() {};

    /** Returns true when the interface is ready to transmit user generated DS485Packets */
    virtual bool IsReady() = 0;

    virtual void SendFrame(DS485CommandFrame& _frame) = 0;

    //------------------------------------------------ Specialized Commands (system)
    /** Returns an vector containing the bus-ids of all modulators present. */
    virtual vector<int> GetModulators() = 0;

    /** Returns a vector conatining the zone-ids of the specified modulator */
    virtual vector<int> GetZones(const int _modulatorID) = 0;
    /** Returns the count of the zones of the specified modulator */
    virtual int GetZoneCount(const int _modulatorID) = 0;
    /** Returns the bus-ids of the devices present in the given zone of the specified modulator */
    virtual vector<int> GetDevicesInZone(const int _modulatorID, const int _zoneID) = 0;
    /** Returns the count of devices present in the given zone of the specified modulator */
    virtual int GetDevicesCountInZone(const int _modulatorID, const int _zoneID) = 0;

    /** Adds the given device to the specified zone. */
    virtual void SetZoneID(const int _modulatorID, const devid_t _deviceID, const int _zoneID) = 0;

    /** Creates a new Zone on the given modulator */
    virtual void CreateZone(const int _modulatorID, const int _zoneID) = 0;

    /** Returns the count of groups present in the given zone of the specifid modulator */
    virtual int GetGroupCount(const int _modulatorID, const int _zoneID) = 0;
    /** Returns the a vector containing the group-ids of the given zone on the specified modulator */
    virtual vector<int> GetGroups(const int _modulatorID, const int _zoneID) = 0;
    /** Returns the count of devices present in the given group */
    virtual int GetDevicesInGroupCount(const int _modulatorID, const int _zoneID, const int _groupID) = 0;
    /** Returns a vector containing the bus-ids of the devices present in the given group */
    virtual vector<int> GetDevicesInGroup(const int _modulatorID, const int _zoneID, const int _groupID) = 0;

    /** Adds a device to a given group */
    virtual void AddToGroup(const int _modulatorID, const int _groupID, const int _deviceID) = 0;
    /** Removes a device from a given group */
    virtual void RemoveFromGroup(const int _modulatorID, const int _groupID, const int _deviceID) = 0;

    /** Adds a user group */
    virtual int AddUserGroup(const int _modulatorID) = 0;
    /** Removes a user group */
    virtual void RemoveUserGroup(const int _modulatorID, const int _groupID) = 0;

    /** Returns the DSID of a given device */
    virtual dsid_t GetDSIDOfDevice(const int _modulatorID, const int _deviceID) = 0;
    /** Returns the DSID of a given modulator */
    virtual dsid_t GetDSIDOfModulator(const int _modulatorID) = 0;

    /** Returns the current power-consumption in mW */
    virtual unsigned long GetPowerConsumption(const int _modulatorID) = 0;

    /** Returns the meter value in Wh */
    virtual unsigned long GetEnergyMeterValue(const int _modulatorID) = 0;
    
    virtual bool GetEnergyBorder(const int _modulatorID, int& _lower, int& _upper) = 0;

    //------------------------------------------------ Device manipulation
    virtual vector<int> SendCommand(DS485Command _cmd, const Set& _set, int _param = -1) = 0;
    virtual vector<int> SendCommand(DS485Command _cmd, const Device& _device, int _param = -1) = 0;
    virtual vector<int> SendCommand(DS485Command _cmd, devid_t _id, uint8_t _modulatorID, int _param = -1) = 0;
    virtual vector<int> SendCommand(DS485Command _cmd, const Zone& _zone, Group& _group, int _param = -1) = 0;
    virtual vector<int> SendCommand(DS485Command _cmd, const Zone& _zone, uint8_t _groupID, int _param = -1) = 0;
  };
}
#endif /* DS485INTERFACE_H_ */
