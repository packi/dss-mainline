/*
 *  serialcom.h
 *  dSS
 *
 *  Created by Patrick Stählin on 6/4/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include <termios.h>

#include <vector>
#include <deque>
#include <string>

using namespace std;

namespace dss {
  
  class SerialComBase {
  public:
    virtual bool Open(const char* _serialPort) =  0;
    
    virtual char GetChar() = 0;
    virtual bool GetCharTimeout(char& _charOut, const int _timeoutSec) = 0;
    virtual void PutChar(const char& _char) = 0;
  }; // SerialComBase
  
  class SerialCom : public SerialComBase {
  private:
    struct termios m_CommSettings;
    int m_Handle;
    string m_PortDevName;
  public:
    SerialCom();
    virtual ~SerialCom();
    
    virtual bool Open(const char* _serialPort);
    
    virtual char GetChar();
    virtual bool GetCharTimeout(char& _charOut, const int _timeoutSec);
    virtual void PutChar(const char& _char);
  }; // SerialCom
  
  class SerialComSim : public SerialComBase {
  private:
    /** Data that will be presented to the user of SerialComBase */
    deque<char> m_IncomingData;
    /** Data that has ben written by the user of SerialComBase */
    deque<char> m_OutgoingData;
  public:
    virtual bool Open(const char* _serialPort);
    
    deque<char>& GetWrittenData();
    void PutSimData(const string& _data);
    
    virtual char GetChar();
    virtual bool GetCharTimeout(char& _charOut, const int _timeoutSec);
    virtual void PutChar(const char& _char);
  }; // SerialComSim
  
}
