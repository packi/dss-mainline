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
  
  class DS485Proxy {
  public:
    double GetValue(const int _parameterID = -1);
    
    void IncrementValue(const Group& _group, const int _parameterID = -1);
    void IncrementValue(const Device& _device, const int _parameterID = -1);
    void IncrementValue(const Set& _set, const int _parameterID = -1);

    void DecrementValue(const Group& _group, const int _parameterID = -1);
    void DecrementValue(const Device& _device, const int _parameterID = -1);
    void DecrementValue(const Set& _set, const int _parameterID = -1);
    
    void StartDim(const Group& _group, const bool _up);
    void StartDim(const Device& _device, const bool _up);
    void StartDim(const Set& _set, const bool _up);
    
    void StopDim(const Group& _group);
    void StopDim(const Device& _device);
    void StopDim(const Set& _set);
    
    void TurnOn(const Group& _group);
    void TurnOn(const Device& _device);
    void TurnOn(const Set& _set);

    void TurnOff(const Group& _group);
    void TurnOff(const Device& _device);
    void TurnOff(const Set& _set);

    void CallScene(const Group& _group);
    void CallScene(const Device& _device);
    
    void CallScene(const Set& _set);

    void SaveScene(const Group& _group);
    void SaveScene(const Device& _device);
    void SaveScene(const Set& _set);

    void UndoScene(const Group& _group);
    void UndoScene(const Device& _device);
    void UndoScene(const Set& _set);
  };
}

#endif
