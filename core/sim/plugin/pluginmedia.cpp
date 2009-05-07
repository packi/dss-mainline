/*
 * pluginmedia.cpp
 *
 *  Created on: Apr 27, 2009
 *      Author: patrick
 */

#include "pluginmedia.h"

void* handleBell(void* ptr) {
  DSIDMedia* mediaPlayer = (DSIDMedia*)ptr;
  if(!mediaPlayer->lastWasOff()) {
    mediaPlayer->powerOff();
    sleep(3);
    mediaPlayer->powerOn();
  }
  return NULL;
} // handleBell
