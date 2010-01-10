/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland

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

#define BOOST_TEST_NO_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <boost/scoped_ptr.hpp>

#include "core/foreach.h"
#include "core/base.h"

#include "core/ds485/ds485.h"
#include "core/ds485types.h"

using namespace dss;

BOOST_AUTO_TEST_SUITE(DS485)

BOOST_AUTO_TEST_CASE(testFrameReader) {
  boost::scoped_ptr<DS485FrameReader> reader(new DS485FrameReader());
  boost::shared_ptr<SerialComSim> simPort(new SerialComSim());
  std::string frame;

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

BOOST_AUTO_TEST_CASE(testFrameReadWrite) {
  DS485CommandFrame cmdFrameOrigin;
  cmdFrameOrigin.getHeader().setBroadcast(true);
  cmdFrameOrigin.getHeader().setCounter(0x02);
  cmdFrameOrigin.getHeader().setDestination(0x03);
  cmdFrameOrigin.getHeader().setSource(0x04);
  
  std::string frameAsString;
  std::vector<unsigned char> frameAsVector = cmdFrameOrigin.toChar();
  uint16_t crc = 0x0000;
  foreach(unsigned char c, frameAsVector) {
    crc = update_crc(crc, c);
    frameAsString.push_back(c);
  }
  unsigned char c = static_cast<unsigned char>(crc & 0xFF);
  frameAsString.push_back(c);
  c = static_cast<unsigned char>((crc >> 8) & 0xFF);
  frameAsString.push_back(c);

  
  boost::shared_ptr<SerialComSim> simPort(new SerialComSim);
  simPort->putSimData(frameAsString);
  
  DS485FrameReader reader;
  reader.setSerialCom(simPort);
  BOOST_CHECK_EQUAL(reader.getNumberOfCRCErrors(), 0);
  BOOST_CHECK_EQUAL(reader.getNumberOfFramesReceived(), 0);
  BOOST_CHECK_EQUAL(reader.getNumberOfIncompleteFramesReceived(), 0);
  
  DS485Frame* pFrame = reader.getFrame(1000);
  DS485CommandFrame* cmdFrame = dynamic_cast<DS485CommandFrame*>(pFrame);
  BOOST_REQUIRE(cmdFrame != NULL);

  BOOST_CHECK_EQUAL(reader.getNumberOfCRCErrors(), 0);
  BOOST_CHECK_EQUAL(reader.getNumberOfFramesReceived(), 1);
  BOOST_CHECK_EQUAL(reader.getNumberOfIncompleteFramesReceived(), 0);

  BOOST_CHECK_EQUAL(cmdFrame->getHeader().isBroadcast(), cmdFrameOrigin.getHeader().isBroadcast());
  BOOST_CHECK_EQUAL(cmdFrame->getHeader().getCounter(),cmdFrameOrigin.getHeader().getCounter());
  BOOST_CHECK_EQUAL(cmdFrame->getHeader().getDestination(),cmdFrameOrigin.getHeader().getDestination());
  BOOST_CHECK_EQUAL(cmdFrame->getHeader().getSource(),cmdFrameOrigin.getHeader().getSource());  
} // testFrameReadWrite

BOOST_AUTO_TEST_CASE(testPayloadIsEmpty) {
  DS485Payload payload;
  BOOST_CHECK_EQUAL(payload.size(), 0);
} // testFrameReadWrite

BOOST_AUTO_TEST_CASE(testPayload) {
  DS485Payload payload;
  payload.add<uint8_t>(0x01);
  BOOST_CHECK_EQUAL(payload.size(), 1);
  payload.add<uint16_t>(0xbeef);
  BOOST_CHECK_EQUAL(payload.size(), 3);
  payload.add<uint32_t>(0xaabbccdd);
  BOOST_CHECK_EQUAL(payload.size(), 7);
  dsid_t dsid(0x001122334455667788ll, 0xeeff9900);
  payload.add(dsid);
  BOOST_CHECK_EQUAL(payload.size(), 19);
  
  PayloadDissector pd(payload);
  BOOST_CHECK_EQUAL(pd.get<uint8_t>(), 0x01);
  BOOST_CHECK_EQUAL(pd.get<uint16_t>(), 0xbeef);
  BOOST_CHECK_EQUAL(pd.get<uint32_t>(), 0xaabbccdd);
  BOOST_CHECK_EQUAL(pd.get<dsid_t>().toString(), dsid.toString());
  BOOST_CHECK(pd.isEmpty());
} // testPayload

BOOST_AUTO_TEST_SUITE_END()
