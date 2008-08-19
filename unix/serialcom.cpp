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

#include <stdexcept>

namespace dss {

  //================================================== SerialCom

  SerialCom::SerialCom() {
    memset(&m_CommSettings, '\0', sizeof(m_CommSettings));
  } // ctor

  SerialCom::~SerialCom() {
    close(m_Handle);
  } // dtor

  bool SerialCom::Open(const char* _serialPort) {
    m_PortDevName = _serialPort;
    m_Handle = open(_serialPort, O_RDWR | O_NOCTTY | O_NDELAY | O_NONBLOCK);
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

    speed_t rate = B115200;
    cfsetispeed(&m_CommSettings, rate);
    cfsetospeed(&m_CommSettings, rate);

    tcflush(m_Handle, TCIOFLUSH); // flush remaining characters

    if(tcsetattr(m_Handle, TCSANOW, &m_CommSettings) == -1) {
      perror("tcsetattr");
      throw runtime_error(string("could not set attributes of port ") + m_PortDevName);
    }
    return true;
  } // Open

  char SerialCom::GetChar() {
    char result;

    int count;
    do {
     count = read(m_Handle, &result, 1);
     if(count < 0 && count != EAGAIN) {
       perror("read failed");
     }
    } while(count != 1);

    return result;
  } // GetChar

  bool SerialCom::GetCharTimeout(char& _chOut, int _timeoutMS) {
    fd_set  fdRead;
    FD_ZERO(&fdRead);
    FD_SET(m_Handle, &fdRead);

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = _timeoutMS * 1000;

    int res = select(m_Handle + 1, &fdRead, NULL, NULL, &timeout );
    if(res == 1) {
      m_ReadWriteLock.Lock();
      read(m_Handle, &_chOut, 1);
      m_ReadWriteLock.Unlock();
      return true;
    } else if(res == 0) {
      // timeout
      return false;
    } else {
      perror("SerialCom::GetCharTimeout() select");
      throw runtime_error("select failed");
    }
    return false;
  }

  void SerialCom::PutChar(const char& _char) {
    int ret = 0;
    while(ret == 0 || ret == EAGAIN) {
      m_ReadWriteLock.Lock();
      ret = write(m_Handle, &_char, 1);
      m_ReadWriteLock.Unlock();

      if(ret != 1) {
        SleepMS(1);
      }
    }
    if(ret != 1) {
      perror("SerialCom::PutChar");
      throw runtime_error("error writing to serial port");
    }
  } // PutChar

  //================================================== SerialComSim

  bool SerialComSim::Open(const char* _serialPort) {
    return true;
  } // Open

  deque<char>& SerialComSim::GetWrittenData() {
    return m_OutgoingData;
  } // GetWrittenData

  void SerialComSim::PutSimData(const string& _data) {
    for(string::const_iterator iChar = _data.begin(), e = _data.end();
        iChar != e; ++iChar)
    {
      m_IncomingData.push_back(*iChar);
    }
  }

  char SerialComSim::GetChar() {
    while(m_IncomingData.size() == 0) {
      SleepMS(100);
    }
    char result = m_IncomingData.front();
    m_IncomingData.pop_front();
    return result;
  }

  bool SerialComSim::GetCharTimeout(char& _charOut, const int _timeoutMS) {
    // TODO: we could/should/want to use an event to wait on incoming data
    if(m_IncomingData.size() != 0) {
      _charOut = GetChar();
      return true;
    } else {
      SleepMS(_timeoutMS);
      if(m_IncomingData.size() != 0) {
        _charOut = GetChar();
        return true;
      }
    }
    return false;
  } // GetCharTimeout

  void SerialComSim::PutChar(const char& _char) {
    m_OutgoingData.push_back(_char);
  } // PutChar

}
