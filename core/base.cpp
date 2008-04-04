/*
 *  base.cpp
 *  dSS
 *
 *  Created by Patrick Stählin on 4/2/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include "base.h"

namespace dss {
  
  template<>
  int StrToInt(const wstring& _strValue) {
    return wcstol(_strValue.c_str(), NULL, 10);  
  }
  
  template<>
  int StrToInt(const wchar_t* _strValue) {
    return wcstol(_strValue, NULL, 10);
  }
  
  
  

  //============================================= Utils
  
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
  }
  
}