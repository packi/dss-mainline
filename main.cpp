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


#include "core/dss.h"
#include "core/logger.h"
#include "tests/tests.h"

#include <ctime>
#include <csignal>
#include <getopt.h>

#ifdef USE_LIBXML
  #include <libxml/tree.h>
  #include <libxml/encoding.h>
#endif

#include <iostream>


using namespace std;

int main (int argc, char * const argv[]) {

  if (!setlocale(LC_CTYPE, "")) {
    cerr << "Can't set the specified locale! Check LANG, LC_CTYPE, LC_ALL." << endl;
    return 1;
  }

  // make sure timezone gets set
  tzset();

  // disable broken pipe signal
#ifndef WIN32
  srand((getpid() << 16) ^ getuid() ^ time(0));
  signal(SIGPIPE, SIG_IGN);
#else
  srand( (int)time( (time_t)NULL ) );
  WSAData dat;
  WSAStartup( 0x1010, &dat );
#endif

#ifdef USE_LIBXML
  // let libXML initialize it's parser
  xmlInitParser();
#endif

  int testFlag = 0;
  string snifferDev;
  bool startSniffer = false;
  int c;
  while(1) {
    static struct option long_options[] =
      {
        // These options set a flag.
        {"dont-runtest", no_argument, &testFlag, 1},
        // These options don't set a flag.
        // We distinguish them by their indices.
        {"sniff",  required_argument,    0, 's'},
        {0, 0, 0, 0}
      };
    // getopt_long stores the option index here.
    int option_index = 0;

    c = getopt_long (argc, argv, "s:",
                    long_options, &option_index);

    if (c == -1) {
      break;
    }

    switch(c)
    {
      case 0:
        break;
      case 's':
        snifferDev = optarg;
        startSniffer = true;
        break;
      default:
        abort();
        break;
    }

  }

  if(testFlag != 1) {
    dss::Tests::Run();
  }

  if(startSniffer) {
#ifndef __APPLE__
    dss::DS485FrameSniffer sniffer(snifferDev);
    sniffer.Run();
    while(true) {
      dss::SleepSeconds(10);
    }
#endif
  } else {
    // start DSS
    dss::DSS::GetInstance()->Run();
  }
  return 0;
}
