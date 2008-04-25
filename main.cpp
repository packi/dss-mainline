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



#include <iostream>

#include "base.h"
#include "dss.h"
#include "core/xmlwrapper.h"
#include "core/jshandler.h"
#include "tests/tests.h"

#include <libxml/tree.h>
#include <libxml/encoding.h>

using namespace std;
using namespace dss;

void testXMLReader();
void testConfig();

int main (int argc, char * const argv[]) {
  
  if (!setlocale(LC_CTYPE, "")) {
    fprintf(stderr, "Can't set the specified locale! "
            "Check LANG, LC_CTYPE, LC_ALL.\n");
    return 1;
  }
  // disable broken pipe signal
#ifndef _WIN32
	signal(SIGPIPE, SIG_IGN);
#endif  
  xmlInitParser();
  
  Tests::Run();
  // start DSS
  dss::DSS::GetInstance()->Run();
  return 0;
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