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
  
  //============================================= String parsing/formatting/conversion
  
  template<>
  int StrToInt(const wstring& _strValue) {
    return wcstol(_strValue.c_str(), NULL, 10);  
  }
  
  template<>
  int StrToInt(const wchar_t* _strValue) {
    return wcstol(_strValue, NULL, 10);
  }
  
  template<>
  int StrToInt(const string& _strValue) {
    return atoi(_strValue.c_str());
  } 
  
  template<>
  int StrToInt(const char* _strValue) {
    return atoi(_strValue);
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
  
  vector<string> SplitString(const string& _source, const char _delimiter) {
    vector<string> result;
    string curString = _source;
    while(curString.size() > 0) {
      string::size_type delimPos = curString.find(_delimiter, 0);
      result.push_back(curString.substr(0, delimPos));
      if(delimPos != string::npos) {
        curString = curString.substr(delimPos+1, string::npos);
      } else {
        break;
      }
    }
    return result;
  } // SplitString
  
  void ReplaceAll(string& s, const string& a, const string& b) {
    for (string::size_type i = s.find(a);
         i != string::npos;
         i = s.find(a, i + b.size()))
      s.replace(i, a.size(), b);
  } // ReplaceAll
  
  string URLDecode(const string& _in) {
    string result = "";
    
    string::size_type lastPos = 0;
    string::size_type pos = _in.find('%', 0);
    while(pos != string::npos && _in.length() > (pos + 2)) {
      if(lastPos != pos) {
        string add = _in.substr(lastPos, pos - lastPos);
        ReplaceAll(add, "+", " ");
        result += add;
      }
      string hex = _in.substr(pos+1, 2);
      char* ptr;
      char tmp = (char)strtol(hex.c_str(), &ptr, 16);
      if(*ptr != '\0') {
        return _in;
      }
      result += tmp;
      lastPos = pos + 3;
      pos = _in.find('%', lastPos);
    }
    if(lastPos < _in.length()) {
      string end = _in.substr(lastPos, string::npos);
      ReplaceAll(end, "+", " ");
      result += end;
    }
    return result;
  }
  
  bool EndsWith( const string& str, const string& searchString ) {
    string::size_type lenStr = str.length();
    string::size_type lenSearch = searchString.length();
    return (lenStr >= lenSearch) &&
    (str.compare( lenStr - lenSearch, lenSearch, searchString ) == 0);
  } // EndsWith
  
  
  bool BeginsWith( const string& str, const string& searchString ) {
    string::size_type lenStr = str.length();
    string::size_type lenSearch = searchString.length();
    return (lenStr >= lenSearch) &&
    (str.compare( 0, lenSearch, searchString ) == 0);
  } // BeginsWith

  
#define                 P_CCITT  0x8408
//#define                 P_CCITT     0x1021
  static int              crc_tabccitt_init       = false;
  static unsigned short   crc_tabccitt[256];
  static void             init_crcccitt_tab( void );
  
  unsigned short update_crc_ccitt( unsigned short crc, char c ) {
    
    unsigned short tmp, short_c;
    
    short_c  = 0x00ff & (unsigned short) c;
    
    if ( ! crc_tabccitt_init ) init_crcccitt_tab();
    
    tmp = (crc >> 8) ^ short_c;
    crc = (crc << 8) ^ crc_tabccitt[tmp];
    
    return crc;
    
  }  /* update_crc_ccitt */
  
  static void init_crcccitt_tab( void ) {
    
    int i, j;
    unsigned short crc, c;
    
    for (i=0; i<256; i++) {
      
      crc = 0;
      c   = ((unsigned short) i) << 8;
      
      for (j=0; j<8; j++) {
        
        if ( (crc ^ c) & 0x8000 ) crc = ( crc << 1 ) ^ P_CCITT;
        else                      crc =   crc << 1;
        
        c = c << 1;
      }
      
      crc_tabccitt[i] = crc;
    }
    
    crc_tabccitt_init = true;
    
  }  /* init_crcccitt_tab */
  /*
  uint16_t CRC16(uint16_t _crc, unsigned const char _data) {
    const uint16_t CRC_CCIT_16 = 0x1021;
    uint16_t crcResult = _crc;
    
    uint16_t tmp = (uint16_t)_data << 8;
    for(int iBit = 0; iBit < 8; iBit++) {
      if(tmp & 0x8000 == 0) {
        crcResult <<= 1;
      } else {
        crcResult = (crcResult << 1)^CRC_CCIT_16;
      }
      tmp <<= 1;
      
    }
    return crcResult;
  }
  */
  
  uint16_t update_crc(uint16_t _crc, const unsigned char& c) {
	int i;
    uint16_t result = _crc ^ c;
  
    for( i=8;i;i--){
	  result = (result >> 1) ^ ((result & 1) ? 0x8408 : 0 );
    }
    return result;
  } // update_crc
  
  uint16_t CRC16(unsigned const char* _data, const int _size) {
    uint16_t result = 0xFFFF;
    for(int iByte = 0; iByte < _size; iByte++) {
      result = update_crc(result, _data[iByte]);
    }
    return result;
  } // CRC16

  //================================================== File utilities
  
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
  
  //================================================== System utilities
  
  void SleepSeconds( const unsigned int _seconds ) {
#ifdef WIN32
    Sleep( _seconds * 1000 );
#else
    sleep( _seconds );
#endif
  } // SleepSeconds
  
  void SleepMS( const unsigned int _ms ) {
#ifdef WIN32
    Sleep( _ms );
#else
    usleep( _ms * 1000 );
#endif
  }
  
  
}