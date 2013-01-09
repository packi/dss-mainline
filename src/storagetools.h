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


namespace dss {

  class PersistentCounter {
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

}

#endif // STORAGETOOLS_H_INCLUDED
