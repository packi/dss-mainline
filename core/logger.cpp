/*
 *  logger.cpp
 *  dSS
 *
 *  Created by Patrick Stählin on 4/2/08.
 *  Copyright:
 *  (c) 2008 by
 *  futureLAB AG
 *  Schwalmenackerstrasse 4
 *  CH-8400 Winterthur / Schweiz
 *  Alle Rechte vorbehalten.
 *  Jede Art der Vervielfaeltigung, Verbreitung,
 *  Auswertung oder Veraenderung - auch auszugsweise -
 *  ist ohne vorgaengige schriftliche Genehmigung durch
 *  die futureLAB AG untersagt.
 *
 * Last change $Date: 2007/11/09 13:18:55 $
 * by $Author: pstaehlin $
 */

#include "logger.h"

#include <cassert>
#include <iostream>

using namespace std;

namespace dss {
  
  Logger* Logger::m_Instance = NULL;
  
  Logger* Logger::GetInstance() {
    if(m_Instance == NULL) {
      m_Instance = new Logger();
    }
    assert(m_Instance != NULL);
    return m_Instance;
  }
  
  void Logger::Log(const string& _message) {
    cout << _message << endl;
  } // Log
  
  void Logger::Log(const char* _message) {
    Log(string(_message));
  } // Log
  
  void Logger::Log(const wchar_t* _message) {
    Log(wstring(_message));
  } // Log
  
  void Logger::Log(const wstring& _message) {
    wcout << _message << endl;
  } // Log

  
}