/*
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

#include <bitset>

#include <string>

#include "core/base.h"
#include "core/model.h"
#include "core/dss.h"
#include "core/xmlwrapper.h"
#include "core/jshandler.h"
#include "tests/tests.h"

#include <ctime>
#include <csignal>

#include <libxml/tree.h>
#include <libxml/encoding.h>

#include <iostream>

#include <boost/scoped_ptr.hpp>


using namespace std;
using namespace dss;

void testXMLReader();
void testConfig();
void testISO();
void testICal();
void testSerial();

int main (int argc, char * const argv[]) {
  
  if (!setlocale(LC_CTYPE, "")) {
    fprintf(stderr, "Can't set the specified locale! "
            "Check LANG, LC_CTYPE, LC_ALL.\n");
    return 1;
  }
  // make sure timezone gets set
  tzset();
  
//  testSerial();
  
  testICal();
  
  // disable broken pipe signal
#ifndef _WIN32
	signal(SIGPIPE, SIG_IGN);
#endif  
  xmlInitParser();
  
  //Tests::Run();
  // start DSS
  dss::DSS::GetInstance()->Run();
  return 0;
}

#include "core/ds485.h"

void testSerial() {
  boost::scoped_ptr<SerialCom> ser(new SerialCom());
  ser->Open("/dev/ttyUSB0");
  int iChar = 0;
  while(true) {
    //cout << ".";
    //flush(cout);
    //ser->PutChar('E');
    char c;
    c = ser->GetChar();
    //if(ser->GetCharTimeout(c, 10)) {
      printf("0x%02X ", (unsigned char)c);
      iChar++;
      if(iChar % 10 == 0) {
        printf("\n");
      }
    //}
    //SleepMS(100);
  }
} // testSerial

void testICal() {
  ICalSchedule s("FREQ=DAILY;INTERVAL=2", "20080505T120000Z");
} // testICal

void testISO() {
  cout << DateTime::FromISO("20080515T120000Z") << endl;
  cout << DateTime::FromISO("20080515T000000Z") << endl;
  cout << DateTime::FromISO("20080101T000000Z") << endl;
  cout << DateTime::FromISO("20080101T120000Z") << endl;
}

void testConfig() {
  Config conf;
  conf.ReadFromXML("/Users/packi/sources/dss/trunk/data/testconfig.xml");
  int bla = conf.GetOptionAs<int>("serverport");
  assert(bla == 8080);
  
  string serverRoot = conf.GetOptionAs<string>("serverroot");
  cout << serverRoot << endl;
  
  assert(conf.GetOptionAs("bla", 1234) == 1234);
  
  conf.SetOptionAs("bla", 1234);
  
  assert(conf.HasOption("bla"));
  assert(conf.GetOptionAs<int>("bla") == 1234);
  assert(conf.GetOptionAs("bla", 4321) == 1234);
}
