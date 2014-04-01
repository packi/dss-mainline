/*
    Copyright (c) 2013 digitalSTROM.org, Zurich, Switzerland

    Author: Michael Tross, aizo GmbH <michael.tross@aizo.com>

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

#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string>

#include "storagetools.h"
#include "dss.h"


namespace dss {

  //================================================== PersistentCounter
  __DEFINE_LOG_CHANNEL__(PersistentCounter, lsInfo);

  extern const char* DataDirectory;

  PersistentCounter::PersistentCounter(const std::string& _name) :
    m_value(0)
  {
    std::string dirname;
    if (DSS::hasInstance()) {
      dirname = DSS::getInstance()->getDataDirectory() + "/store/";
    } else {
      dirname = std::string(DataDirectory) + "/store/";
    }
    int ret = mkdir(dirname.c_str(), S_IRWXU | S_IRGRP);
    if (ret < 0) {
      // do not warn about already existing directory
      if (errno != EEXIST) {
        Logger::getInstance()->log("Create directory '" + dirname + "' failed: " +
            std::string(strerror(errno)), lsFatal);
      }
    }
    m_filename = dirname + "count." + _name;
    load();
  }

  void PersistentCounter::increment() {
    m_value ++;
    store();
  }

  void PersistentCounter::load() {
    FILE *fin = fopen(m_filename.c_str(), "r");
    if (NULL == fin) {
      return;
    }
    unsigned long long value;
    if (fscanf(fin, "%llu", &value) == 1) {
      m_value = value;
    }
    fclose(fin);
  }

  void PersistentCounter::store() {
    std::string tempFile = m_filename + ".tmp";
    FILE *fout = fopen(tempFile.c_str(), "w");
    if (NULL == fout) {
      return;
    }
    fprintf(fout, "%llu\n", (unsigned long long) m_value);
    fclose(fout);
    int ret = rename(tempFile.c_str(), m_filename.c_str());
    if (ret < 0) {
      Logger::getInstance()->log("Copying to final destination (" + m_filename + ") failed: " +
          std::string(strerror(errno)), lsFatal);
    }
  }

}
