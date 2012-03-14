/*
    Copyright (c) 2010,2012 digitalSTROM.org, Zurich, Switzerland

    Author: Patrick Staehlin, futureLAB AG <pstaehlin@futurelab.ch>
            Michael Tross, aizo GmbH <michael.tross@aizo.com>

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
#include "stdio.h"
#include "string.h"

#ifdef HAVE_BACKTRACE
  #include <execinfo.h>
  #include <cxxabi.h>
#endif

#include "src/base.h"
#include "src/logger.h"

namespace dss {

  void Backtrace::logBacktrace() {
#if defined(HAVE_BACKTRACE) && defined(HAVE_BACKTRACE_SYMBOLS)
    const int kBacktraceSize = 50;
    void* btBuffer[kBacktraceSize];
    int backtraceSize = backtrace(btBuffer, kBacktraceSize);
    char** backtraceStrings = backtrace_symbols(btBuffer, backtraceSize);
    if(backtraceStrings == NULL) {
      Logger::getInstance()->log("Couldn't get backtrace symbols", lsFatal);
    } else {
      char temp[4096];
      char addr[20];
      char offset[20];
      char* symname;
      char* demangled;
      size_t size;
      int status;
      for (int i = 1; i < backtraceSize; i++) {
        offset[0] = 0;
        addr[0] = 0;
        demangled = NULL;
        if (3 == sscanf(backtraceStrings[i], "%*[^(](%4095[^+]+%[^)]) %s", temp, offset, addr)) {
          symname = temp;
          if (NULL != (demangled = abi::__cxa_demangle(temp, NULL, &size, &status))) {
            symname = demangled;
          }
        } else if (3 == sscanf(backtraceStrings[i], "%*d %*s %s %s %*s %s", addr, temp, offset)) {
          symname = temp;
          if (NULL != (demangled = abi::__cxa_demangle(temp, NULL, &size, &status))) {
            symname = demangled;
          }
        } else if (2 == sscanf(backtraceStrings[i], "%s %s", temp, addr)) {
          symname = temp;
        } else {
          symname = backtraceStrings[i];
        }

        Logger::getInstance()->log("Backtrace[" + intToString(i) + "]    " +
            symname + "    " + backtraceStrings[i], lsFatal);

        if (demangled) {
          free(demangled);
        }
      }
    }
    free(backtraceStrings);
  }
#endif
}
