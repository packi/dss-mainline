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

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "backtrace.h"

#if (defined HAVE_BACKTRACE) && (defined HAVE_BACKTRACE_SYMBOLS)
  #include <execinfo.h>
#endif

#include "src/base.h"
#include "src/logger.h"

namespace dss {

  void Backtrace::logBacktrace() {
#if (defined HAVE_BACKTRACE) && (defined HAVE_BACKTRACE_SYMBOLS)
    const int kBacktraceSize = 50;
    void* btBuffer[kBacktraceSize];
    int backtraceSize = backtrace(btBuffer, kBacktraceSize);
    char** backtraceStrings = backtrace_symbols(btBuffer, backtraceSize);
    if(backtraceStrings == NULL) {
      Logger::getInstance()->log("Couldn't get backtrace symbols", lsFatal);
    } else {
      for(int iString = 0; iString < backtraceSize; iString++) {
        Logger::getInstance()->log("Backtrace[" + intToString(iString) + "]: " + backtraceStrings[iString], lsFatal);
      }
    }
    free(backtraceStrings);
#endif
  } // logBacktrace

}