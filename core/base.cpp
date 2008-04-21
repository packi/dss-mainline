/*
 *  base.cpp
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
 * Last change $Date$
 * by $Author$
 */

#include "base.h"

#include <sys/stat.h>
#include <sstream>

namespace dss {
  
  //============================================= Utils
  
  template<>
  int StrToInt(const wstring& _strValue) {
    return wcstol(_strValue.c_str(), NULL, 10);  
  }
  
  template<>
  int StrToInt(const wchar_t* _strValue) {
    return wcstol(_strValue, NULL, 10);
  }
  
  string IntToString(const int _int) {
    stringstream sstream;
    sstream << _int;
    return sstream.str();
  } // IntToString
  
  const char* theISOFormatString = "%Y-%m-%d %H:%M:%S";
  
  template <>
  string DateToISOString( const struct tm* _dateTime ) {
    char buf[ 20 ];
    strftime( buf, 20, theISOFormatString, _dateTime );
    string result = buf;
    return result;
  } // DateToISOString
  
  
  struct tm DateFromISOString( const char* _dateTimeAsString ) {
    struct tm result;
    strptime( _dateTimeAsString, theISOFormatString, &result );
    return result;
  } // DateFromISOString
 
  
  template <>
  wstring DateToISOString(const struct tm* _dateTime) {
    return FromUTF8(DateToISOString<string>(_dateTime));
  } // DateToISOString
  
  const int ConversionBufferSize = 1024;

  /** Takes a string in utf-8 format and converts it to a wide-string */  
  const wstring FromUTF8(const char* _utf8string, int _len) {
    wchar_t buffer[ConversionBufferSize];
    wstring result;
    mbstate_t state;
    const char* ptr;
    
    ptr = &_utf8string[0];
    memset(&state, '\0', sizeof(mbstate_t));  
    while(true) {
      size_t wlen = mbsnrtowcs(buffer, &ptr, 
                               _len,
                               ConversionBufferSize,  // max chars to return
                               &state);
      if(wlen < 0) {
        throw new DSSException("FromUTF8: Illegal sequence encountered");
      } else if(wlen == 0 ) {
        break;
      }
      result.append(buffer, wlen);
    }
    return result;
  } // FromUTF8
  
  const string ToUTF8(const wchar_t* _wcharString, int _len) {
    char buffer[ConversionBufferSize];
    string result;
    mbstate_t state;
    const wchar_t* ptr;
    
    ptr = &_wcharString[0];
    memset(&state, '\0', sizeof(mbstate_t));  
    while(true) {
      size_t len = wcsnrtombs(buffer, &ptr, _len, ConversionBufferSize, &state);
      if(len < 0) {
        throw new DSSException("ToUF8: Error");
      } else if(len == 0) {
        break;
      }
      result.append(buffer, len);
    }
    return result;
  } // ToUTF8
  
  const wstring FromUTF8(const string& _utf8string) {
    return FromUTF8(_utf8string.c_str(), _utf8string.size());
  }
  
  const string ToUTF8(const wstring& _wcharString) {
    return ToUTF8(_wcharString.c_str(), _wcharString.size());
  }

  
  bool FileExists( const string& _fileName ) {
    return FileExists(_fileName.c_str());
  } // FileExists
  
  bool FileExists( const char* _fileName ) {
    #ifdef WIN32
      return (FileAge( _fileName ) != -1);
    #else
      #ifdef __GNU__
        return euidaccess( _fileName, F_OK ) == 0;
      #else
        struct stat buf;
        if( lstat( _fileName, &buf ) != -1 ) {
          int fileType = (buf.st_mode & S_IFMT);
          return fileType  == S_IFREG;
        } else {
          return false;
        }
      #endif
    #endif
  } // FileExists
  
}