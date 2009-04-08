/*
 *  serialcom.h
 *  dSS
 *
 *  Created by Patrick Stählin on 6/4/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef _SERIAL_COM_H_INCLUDED
#define _SERIAL_COM_H_INCLUDED

#include <termios.h>

#include <vector>
#include <deque>
#include <string>

#include "../core/mutex.h"

using namespace std;

namespace dss {

  class SerialComBase {
  public:
    virtual bool Open(const char* _serialPort) =  0;

    virtual char GetChar() = 0;
    virtual bool GetCharTimeout(char& _charOut, const int _timeoutMS) = 0;
    virtual void PutChar(const char& _char) = 0;

    virtual ~SerialComBase() {};
  }; // SerialComBase

  class SerialCom : public SerialComBase {
  public:
    typedef enum { sp9600, sp115200 } SerialSpeed;
  private:
    struct termios m_CommSettings;
    int m_Handle;
    string m_PortDevName;
    Mutex m_ReadWriteLock;
    SerialSpeed m_Speed;
    bool m_Blocking;
  public:
    SerialCom();
    virtual ~SerialCom();

    virtual bool Open(const char* _serialPort);

    virtual char GetChar();
    virtual bool GetCharTimeout(char& _charOut, const int _timeoutMS);
    virtual void PutChar(const char& _char);

    void SetSpeed(SerialSpeed _value) { m_Speed = _value; }
    void SetBlocking(bool _value) { m_Blocking = _value; }
  }; // SerialCom

  class SerialComSim : public SerialComBase {
  private:
    /** Data that will be presented to the user of SerialComBase */
    deque<char> m_IncomingData;
    /** Data that has ben written by the user of SerialComBase */
    deque<char> m_OutgoingData;
  public:
    virtual ~SerialComSim() {};

    virtual bool Open(const char* _serialPort);

    deque<char>& GetWrittenData();
    void PutSimData(const string& _data);

    virtual char GetChar();
    virtual bool GetCharTimeout(char& _charOut, const int _timeoutMS);
    virtual void PutChar(const char& _char);
  }; // SerialComSim

}

#endif
