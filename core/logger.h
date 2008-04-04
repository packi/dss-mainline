/*
 *  logger.h
 *  dSS
 *
 *  Created by Patrick Stählin on 4/2/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef LOGGER_H_INLUDED
#define LOGGER_H_INLUDED

#include <string>

using namespace std;

namespace dss {
  
  class Logger {
  private:
    static Logger* m_Instance;
    
    Logger() {};
  public:
    static Logger* GetInstance();
    
    void Log(const string& _message);
    void Log(const char* _message);
    void Log(const wchar_t* _message);
    void Log(const wstring& _message);
  }; // Logger
}

#endif