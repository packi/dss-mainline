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

#include "serialcom.h"

#include "../core/base.h"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <cstring>
#include <cstdio>

#include <stdexcept>

namespace dss {

  //================================================== SerialCom

  SerialCom::SerialCom()
  : m_Handle(-1),
    m_Speed(sp115200),
    m_Blocking(false)
  {
    memset(&m_CommSettings, '\0', sizeof(m_CommSettings));
  } // ctor

  SerialCom::~SerialCom() {
    close(m_Handle);
  } // dtor

  bool SerialCom::open(const char* _serialPort) {
    m_PortDevName = _serialPort;
    int flags = O_RDWR | O_NOCTTY;
    if(!m_Blocking) {
      flags |= O_NDELAY | O_NONBLOCK;
    }
    m_Handle = ::open(_serialPort, flags);
    if(m_Handle == -1) {
      perror("serial");
      throw std::runtime_error(std::string("could not open port ") + m_PortDevName);
    }


    tcgetattr(m_Handle, &m_CommSettings);

    // 8-N-1
    m_CommSettings.c_iflag &= ~( IGNBRK | BRKINT | PARMRK | ISTRIP |
                                 INLCR | IGNCR | ICRNL | IXON );
		m_CommSettings.c_oflag &= ~OPOST;
		m_CommSettings.c_lflag &= ~( ECHO | ECHONL | ICANON | ISIG | IEXTEN );
		m_CommSettings.c_cflag &= ~( CSIZE | PARENB | CRTSCTS );
		m_CommSettings.c_cflag |= CS8 | CREAD | CLOCAL ;
		m_CommSettings.c_cc[VMIN] = 0;
		m_CommSettings.c_cc[VTIME] = 0;

    speed_t rate;
    if(m_Speed == sp115200) {
      rate = B115200;
    } else if(m_Speed == sp9600) {
      rate = B9600;
    } else {
      throw std::runtime_error("Invalid value of speed");
    }
    if(cfsetispeed(&m_CommSettings, rate) == -1) {
      perror("cfsetispeed");
      throw std::runtime_error(std::string("could not set input speed of port ") + m_PortDevName);
    }
    if(cfsetospeed(&m_CommSettings, rate) == -1) {
      perror("cfsetospeed");
      throw std::runtime_error(std::string("could not set ouput speed of port ") + m_PortDevName);
    }

    // flush remaining characters
    if(tcflush(m_Handle, TCIOFLUSH) == -1) {
      perror("tcflush");
      throw std::runtime_error(std::string("could not flush port ") + m_PortDevName);
    }

    if(tcsetattr(m_Handle, TCSANOW, &m_CommSettings) == -1) {
      perror("tcsetattr");
      throw std::runtime_error(std::string("could not set attributes of port ") + m_PortDevName);
    }
    return true;
  } // open

  char SerialCom::getChar() {
    char result;

    while(!getCharTimeout(result, 10000)) {
    }

    return result;
  } // getChar

  bool SerialCom::getCharTimeout(char& _chOut, int _timeoutMS) {
    fd_set  fdRead;
    FD_ZERO(&fdRead);
    FD_SET(m_Handle, &fdRead);

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = _timeoutMS * 1000;

    int res = select(m_Handle + 1, &fdRead, NULL, NULL, &timeout);
    if(res == 1) {
      m_ReadWriteLock.lock();
      res = read(m_Handle, &_chOut, 1);
      m_ReadWriteLock.unlock();
      if(res == 1) {
        return true;
      } else {
        if((res == EWOULDBLOCK) || (res == EAGAIN)) {
          return false;
        } else {
          close(m_Handle);
          m_Handle = -1;
          perror("SerialCom::getCharTimeout read");
          throw std::runtime_error("read failed");
        }
      }
    } else if(res == 0) {
      // timeout
      return false;
    } else {
      close(m_Handle);
      m_Handle = -1;
      perror("SerialCom::getCharTimeout() select");
      throw std::runtime_error("select failed");
    }
    return false;
  }

  void SerialCom::putChar(const char& _char) {
    int ret = 0;
    while(ret == 0 || ret == EAGAIN) {
      m_ReadWriteLock.lock();
      ret = write(m_Handle, &_char, 1);
      m_ReadWriteLock.unlock();

      if(ret != 1) {
        sleepMS(1);
      }
    }
    if(ret != 1) {
      close(m_Handle);
      m_Handle = -1;
      perror("SerialCom::putChar");
      throw std::runtime_error("error writing to serial port");
    }
  } // putChar

  //================================================== SerialComSim

  bool SerialComSim::open(const char* _serialPort) {
    return true;
  } // open

  std::deque<char>& SerialComSim::getWrittenData() {
    return m_OutgoingData;
  } // getWrittenData

  void SerialComSim::putSimData(const std::string& _data) {
    for(std::string::const_iterator iChar = _data.begin(), e = _data.end();
        iChar != e; ++iChar)
    {
      m_IncomingData.push_back(*iChar);
    }
  }

  char SerialComSim::getChar() {
    while(m_IncomingData.size() == 0) {
      sleepMS(100);
    }
    char result = m_IncomingData.front();
    m_IncomingData.pop_front();
    return result;
  }

  bool SerialComSim::getCharTimeout(char& _charOut, const int _timeoutMS) {
    // TODO: we could/should/want to use an event to wait on incoming data
    if(m_IncomingData.size() != 0) {
      _charOut = getChar();
      return true;
    } else {
      sleepMS(_timeoutMS);
      if(m_IncomingData.size() != 0) {
        _charOut = getChar();
        return true;
      }
    }
    return false;
  } // getCharTimeout

  void SerialComSim::putChar(const char& _char) {
    m_OutgoingData.push_back(_char);
  } // putChar

}
