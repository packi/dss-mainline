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

#ifndef _SERIAL_COM_H_INCLUDED
#define _SERIAL_COM_H_INCLUDED

#include <termios.h>

#include <deque>
#include <string>

#include "../core/mutex.h"

namespace dss {

  class SerialComBase {
  public:
    virtual bool open(const char* _serialPort) =  0;

    virtual char getChar() = 0;
    virtual bool getCharTimeout(char& _charOut, const int _timeoutMS) = 0;
    virtual void putChar(const char& _char) = 0;

    virtual ~SerialComBase() {};
  }; // SerialComBase

  class SerialCom : public SerialComBase {
  public:
    typedef enum { sp9600, sp115200 } SerialSpeed;
  private:
    struct termios m_CommSettings;
    int m_Handle;
    std::string m_PortDevName;
    Mutex m_ReadWriteLock;
    SerialSpeed m_Speed;
    bool m_Blocking;
  public:
    SerialCom();
    virtual ~SerialCom();

    virtual bool open(const char* _serialPort);

    virtual char getChar();
    virtual bool getCharTimeout(char& _charOut, const int _timeoutMS);
    virtual void putChar(const char& _char);

    void setSpeed(SerialSpeed _value) { m_Speed = _value; }
    void setBlocking(bool _value) { m_Blocking = _value; }
  }; // SerialCom

  class SerialComSim : public SerialComBase {
  private:
    /** Data that will be presented to the user of SerialComBase */
    std::deque<char> m_IncomingData;
    /** Data that has ben written by the user of SerialComBase */
    std::deque<char> m_OutgoingData;
  public:
    virtual ~SerialComSim() {};

    virtual bool open(const char* _serialPort);

    std::deque<char>& getWrittenData();
    void putSimData(const std::string& _data);

    virtual char getChar();
    virtual bool getCharTimeout(char& _charOut, const int _timeoutMS);
    virtual void putChar(const char& _char);
  }; // SerialComSim

}

#endif
