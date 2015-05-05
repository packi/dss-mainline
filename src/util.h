/*
    Copyright (c) 2013 digitalSTROM.org, Zurich, Switzerland

    Authors: Christian Hitz, aizo AG <christian.hitz@aizo.com>
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

#ifndef __DSS_UTIL_H_INCLUDED__
#define __DSS_UTIL_H_INCLUDED__

#include <string>
#include <vector>
#include <inttypes.h>
#include <boost/shared_ptr.hpp>

namespace dss {

  class Apartment;
  class StructureModifyingBusInterface;

  void synchronizeGroups(Apartment* _apartment, StructureModifyingBusInterface* _interface);

  std::string escapeHTML(std::string _input);
  std::string unescapeHTML(std::string _input);

  std::pair<uint8_t, uint8_t> getOutputChannelIdAndSize(std::string _channelName);
  std::string getOutputChannelName(uint8_t _channel);
  uint16_t convertToOutputChannelValue(uint8_t channel, double value);
  double convertFromOutputChannelValue(uint8_t channel, uint16_t value);
  bool saveValidatedXML(const std::string& _fileName, const std::string& _targetFile);
  std::vector<int> parseBitfield(const uint8_t *_bitfield, int _bits);
}

#endif
