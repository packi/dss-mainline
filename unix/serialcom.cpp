/*
 *  serialcom.cpp
 *  dSS
 *
 *  Created by Patrick Stählin on 6/4/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include "serialcom.h"

#include "../core/base.h"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <cstring>

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
      throw runtime_error(string("could not open port ") + m_PortDevName);
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
      throw runtime_error("Invalid value of speed");
    }
    if(cfsetispeed(&m_CommSettings, rate) == -1) {
      perror("cfsetispeed");
      throw runtime_error(string("could not set input speed of port ") + m_PortDevName);
    }
    if(cfsetospeed(&m_CommSettings, rate) == -1) {
      perror("cfsetospeed");
      throw runtime_error(string("could not set ouput speed of port ") + m_PortDevName);
    }

    // flush remaining characters
    if(tcflush(m_Handle, TCIOFLUSH) == -1) {
      perror("tcflush");
      throw runtime_error(string("could not flush port ") + m_PortDevName);
    }

    if(tcsetattr(m_Handle, TCSANOW, &m_CommSettings) == -1) {
      perror("tcsetattr");
      throw runtime_error(string("could not set attributes of port ") + m_PortDevName);
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
          throw runtime_error("read failed");
        }
      }
    } else if(res == 0) {
      // timeout
      return false;
    } else {
      close(m_Handle);
      m_Handle = -1;
      perror("SerialCom::getCharTimeout() select");
      throw runtime_error("select failed");
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
      throw runtime_error("error writing to serial port");
    }
  } // putChar

  //================================================== SerialComSim

  bool SerialComSim::open(const char* _serialPort) {
    return true;
  } // open

  deque<char>& SerialComSim::getWrittenData() {
    return m_OutgoingData;
  } // getWrittenData

  void SerialComSim::putSimData(const string& _data) {
    for(string::const_iterator iChar = _data.begin(), e = _data.end();
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
