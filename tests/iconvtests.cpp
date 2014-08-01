/*
    Copyright (c) 2012 digitalSTROM.org, Zurich, Switzerland

    Author: Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

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

#include "../src/base.h"
#include "../src/stringconverter.h"

using namespace dss;

BOOST_AUTO_TEST_SUITE(Iconv)

BOOST_AUTO_TEST_CASE(testUTF8valid) {
  StringConverter st("UTF-8", "UTF-8");
  BOOST_CHECK_NO_THROW(st.convert("Тест кодировки UTF-8"));
}

BOOST_AUTO_TEST_CASE(testUTF8invalid) {
  StringConverter st("UTF-8", "UTF-8");
  const uint8_t str[] = { 0xf4, 0xc5, 0xd3, 0xd4, 0x20, 0xce, 0xc5, 0xd7, 0xc5,
                          0xd2, 0xce, 0xcf, 0xca, 0x20, 0xcb, 0xcf, 0xc4, 0xc9,
                          0xd2, 0xcf, 0xd7, 0xcb, 0xc9, 0x00 };

  BOOST_CHECK_THROW(st.convert(reinterpret_cast<const char *>(str)),
                    DSSException);
}

BOOST_AUTO_TEST_SUITE_END()

