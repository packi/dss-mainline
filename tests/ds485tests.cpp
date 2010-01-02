/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland
    Copyright (c) 2009 futureLAB AG, Winterthur, Switzerland

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

#define BOOST_TEST_NO_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <boost/scoped_ptr.hpp>

#include "core/ds485/ds485.h"

using namespace dss;

BOOST_AUTO_TEST_SUITE(DS485)

BOOST_AUTO_TEST_CASE(testFrameReader) {
  boost::scoped_ptr<DS485FrameReader> reader(new DS485FrameReader());
  boost::shared_ptr<SerialComSim> simPort(new SerialComSim());
  std::string frame;
/*
  frame.push_back('\xFD');
  frame.push_back('\x05');
  frame.push_back('\x00');
  frame.push_back('\x71');
  frame.push_back('\x00');
  frame.push_back('\x99');
  frame.push_back('\x64');
*/
  /* Working frame

  frame.push_back('\xFD');
  frame.push_back('\x01');
  frame.push_back('\x00');
  frame.push_back('\x14');
  frame.push_back('\xce');
  frame.push_back('\x01');
  frame.push_back('\x00');
  frame.push_back('\x00');
  frame.push_back('\x39');
  frame.push_back('\x24');
   */

  /* Captured frames */
  frame.push_back('\xFD');
  frame.push_back('\x3F');
  frame.push_back('\x7E');
  frame.push_back('\xD7');
  frame.push_back('\x7E');
  frame.push_back('\x88');
  frame.push_back('\xFD');
  frame.push_back('\xF7');
  frame.push_back('\xF7');
  frame.push_back('\xB7');
  frame.push_back('\x78');
  frame.push_back('\x50');
  frame.push_back('\x05');
  frame.push_back('\x5E');
  frame.push_back('\xF7');
  frame.push_back('\x77');
  frame.push_back('\xF7');
  frame.push_back('\x87');
  frame.push_back('\xD6');
  frame.push_back('\xF7');
  frame.push_back('\xF7');
  frame.push_back('\x17');
  frame.push_back('\x72');
  frame.push_back('\x56');
  frame.push_back('\x05');
  frame.push_back('\xD6');
  frame.push_back('\xD7');
  frame.push_back('\x77');
  frame.push_back('\x77');
  frame.push_back('\xC8');
  frame.push_back('\xFD');
  frame.push_back('\xF7');
  frame.push_back('\xF7');
  frame.push_back('\x77');
  frame.push_back('\x63');
  frame.push_back('\x54');
  frame.push_back('\xFF');
  frame.push_back('\x05');
  frame.push_back('\xF8');
  frame.push_back('\x7F');
  frame.push_back('\x7F');
  frame.push_back('\x3F');
  frame.push_back('\xAE');
  frame.push_back('\x7E');
  frame.push_back('\x88');
  frame.push_back('\xFD');
  frame.push_back('\x7E');
  frame.push_back('\xFE');
  frame.push_back('\xFF');
  frame.push_back('\x7F');
  frame.push_back('\x0F');
  frame.push_back('\xD2');
  frame.push_back('\xFE');
  frame.push_back('\x05');
  frame.push_back('\xFC');
  frame.push_back('\x7F');
  frame.push_back('\xF2');
  frame.push_back('\xFE');
  frame.push_back('\xDF');
  frame.push_back('\x7E');
  frame.push_back('\x88');

  simPort->putSimData(frame);
  reader->setSerialCom(simPort);
  delete reader->getFrame(1000);
} // testFrameReader

BOOST_AUTO_TEST_SUITE_END()
