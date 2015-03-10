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

#ifndef STORAGETOOLS_H_INCLUDED
#define STORAGETOOLS_H_INCLUDED

#include <stdint.h>
#include <string>
#include <iostream>
#include "base.h"
#include "logger.h"

namespace dss {

  class PersistentCounter {
    __DECL_LOG_CHANNEL__
  private:
    uint64_t m_value;
    std::string m_filename;
    void load();
  public:
    PersistentCounter(const std::string& _name);
    void increment();
    void store();
    void setValue(uint64_t _val) { m_value = _val; store(); }
    uint64_t getValue() const { return m_value; }
  };

  class BasePersistentValue {
  public:
    virtual ~BasePersistentValue();
  protected:
    void setup(const std::string& _name);
    const std::string& getFilename() const;
    void moveFile(const std::string& _temp, const std::string& _target);
  private:
    std::string m_filename;
  };

  template <class VALTYPE>
  class PersistentValue : public BasePersistentValue {
  public:
    PersistentValue(const std::string& _name, VALTYPE _uninitValue) :
      m_value(_uninitValue) {
      setup(_name);
      load();
    }

    void setValue(VALTYPE _value) {
      m_value = _value;
      store();
    }

    VALTYPE getValue() const {
      return m_value;
    }

    void store() {
      std::string tmpOut = getFilename() + ".tmp";
      std::ofstream ofs(tmpOut.c_str());
      if(ofs) {
        ofs << m_value << std::endl;
        ofs.close();
        syncFile(tmpOut);
        if (validatePersistenData(tmpOut)) {
          std::string finalOutput = getFilename();
          moveFile(tmpOut, finalOutput);
        }
      }
    }

    void store(const std::string& _name) {
      setup(_name);
      store();
    }

  private:
    void load() {
      std::ifstream input_file(getFilename().c_str());
      if (input_file.is_open()) {
        input_file >> m_value;
      }
      input_file.close();
    }

    bool validatePersistenData(const std::string& _file) {
      std::ifstream tempIfs(_file.c_str());
      if (tempIfs.is_open()) {
        VALTYPE value;
        tempIfs >> value;
        tempIfs.close();
        return (value == m_value);
      }
      return false;
    }

    VALTYPE m_value;
  };
}

#endif // STORAGETOOLS_H_INCLUDED
