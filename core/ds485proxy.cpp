/*
 *  ds485proxy.cpp
 *  dSS
 *
 *  Created by Patrick Stählin on 4/18/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include "ds485proxy.h"

#include "model.h"

namespace dss {

  typedef hash_map<const Modulator*, Set> HashMapModulatorSet;
  
  HashMapModulatorSet SplitByModulator(Set& _set) {
    HashMapModulatorSet result;
    for(int iDevice = 0; iDevice < _set.Length(); iDevice++) {
      DeviceReference& dev = _set.Get(iDevice);
      Modulator& mod = dev.GetDevice().GetApartment().GetModulator(dev.GetDevice().GetModulatorID());
      result[&mod].AddDevice(dev);
    }
    return result;
  } // SplitByModulator
  
  typedef pair<vector<Group*>, Set> FittingResultPerModulator;

  
  FittingResultPerModulator BestFit(const Modulator& _modulator, Set& _set) {
    Set workingCopy = _set;
    
    vector<Group*> unsuitableGroups;
    vector<Group*> fittingGroups;
    Set singleDevices;
    
    while(!workingCopy.IsEmpty()) {
      DeviceReference& ref = workingCopy.Get(0);
      workingCopy.RemoveDevice(ref);
      
      bool foundGroup = false;
      for(int iGroup = 0; iGroup < ref.GetDevice().GetGroupsCount(); iGroup++) {
        Group& g = ref.GetDevice().GetGroupByIndex(iGroup);
        
        // continue if already found unsuitable
        if(find(unsuitableGroups.begin(), unsuitableGroups.end(), &g) != unsuitableGroups.end()) {
          continue;
        }
        
        // see if we've got a fit
        bool groupFits = true;
        Set devicesInGroup = _modulator.GetDevices().GetByGroup(g);
        for(int iDevice = 0; iDevice < devicesInGroup.Length(); iDevice++) {
          if(!_set.Contains(devicesInGroup.Get(iDevice))) {
            unsuitableGroups.push_back(&g);
            groupFits = false;
            break;
          }
        }
        if(groupFits) {
          foundGroup = true;
          fittingGroups.push_back(&g);
          while(!devicesInGroup.IsEmpty()) {
            workingCopy.RemoveDevice(devicesInGroup.Get(0));
          }
          break;
        }        
      }
      
      // if no fitting group found
      if(!foundGroup) {
        singleDevices.AddDevice(ref);
      }
    }
    return FittingResultPerModulator(fittingGroups, singleDevices);
  }
  
  FittingResult DS485Proxy::BestFit(Set& _set) {
    FittingResult result;
    HashMapModulatorSet modulatorToSet = SplitByModulator(_set);
    
    for(HashMapModulatorSet::iterator it = modulatorToSet.begin(); it != modulatorToSet.end(); ++it) {
      result[it->first] = dss::BestFit(*(it->first), it->second); 
    }
    
    return result;
  }
  
  void DS485Proxy::SendCommand(DS485Command _cmd, Set& _set) {
    BestFit(_set);
  } // SendCommand
  
  void DS485Proxy::SendCommand(DS485Command _cmd, Device& _device) {
  } // SendCommand
  
  void DS485Proxy::SendCommand(DS485Command _cmd, devid_t _id) {
  } // SendCommand

  bool DS485Proxy::IsSimAddress(devid_t _addr) {
    return true;
  } // IsSimAddress

}
