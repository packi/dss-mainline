/*
 * pluginmedia.cpp
 *
 *  Created on: Apr 27, 2009
 *      Author: patrick
 */

#include "pluginmedia.h"

void* handleBell(void* ptr) {
  DSIDMedia* mediaPlayer = (DSIDMedia*)ptr;
  mediaPlayer->powerOn();
  sleep(3);
  mediaPlayer->powerOff();
  return NULL;
} // handleBell
