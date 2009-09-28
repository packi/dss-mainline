/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland
    Copyright (c) 2009 futureLAB AG, Winterthur, Switzerland

    This file is part of digitalSTROM Server.

    digitalSTROM Server is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    digitalSTROM Server is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with digitalSTROM Server. If not, see <http://www.gnu.org/licenses/>.

    Last change $Date$
    by $Author$
*/


#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "core/base.h"
#include "core/dss.h"
#include "core/logger.h"
#include "unix/ds485.h"
#ifdef WITH_TESTS
#include "tests/tests.h"
#endif

#include <ctime>
#include <csignal>
#include <getopt.h>

#include <boost/program_options.hpp>

#ifdef USE_LIBXML
  #include <libxml/tree.h>
  #include <libxml/encoding.h>
#endif

#include <iostream>

using namespace std;
namespace po = boost::program_options;

pair<string, string> parse_prop(const string& s) {
    if (s.find("--prop") == 0) {
      return make_pair("prop", s.substr(7));
    } else {
      return make_pair(string(), string());
    }
} // parse_prop

int main (int argc, char* argv[]) {

  if (!setlocale(LC_CTYPE, "")) {
    cerr << "Can't set the specified locale! Check LANG, LC_CTYPE, LC_ALL." << endl;
    return 1;
  }

  // make sure timezone gets set
  tzset();

  char* tzNameCopy = strdup("GMT");
  tzname[0] = tzname[1] = tzNameCopy;
  timezone = 0;
  daylight = 0;

  setenv("TZ", "UTC", 1);


#ifndef WIN32
  srand((getpid() << 16) ^ getuid() ^ time(0));
  // disable broken pipe signal
  signal(SIGPIPE, SIG_IGN);
#else
  srand( (int)time( (time_t)NULL ) );
  WSAData dat;
  WSAStartup( 0x1010, &dat );
#endif


  vector<string> properties;
#ifdef USE_LIBXML
  // let libXML initialize its parser
  xmlInitParser();
#endif


  po::options_description desc("Allowed options");
  desc.add_options()
      ("help", "produce help message")
#ifdef WITH_TESTS
      ("dont-runtests", po::value<bool>(), "if set, no tests will be run")
      ("quit-after-tests", po::value<bool>(), "if set, the application will terminate after running its tests")
#endif
#ifndef __APPLE__
      ("sniff,s", po::value<string>(), "start the ds485 sniffer")
#endif
      ("prop", po::value<vector<string> >(), "sets a property")
      ("version,v", "print version information and exit")
  ;

  po::variables_map vm;
  po::store(po::command_line_parser(argc, argv).options(desc).extra_parser(parse_prop)
          .run(), vm);
  po::notify(vm);

  if (vm.count("help")) {
      cout << desc << "\n";
      return 1;
  }

  if (vm.count("version")) {
    std::cout << dss::DSS::versionString() << std::endl;
    return 1;
  }

  bool runTests = true;
  if(vm.count("dont-runtests")) {
    runTests = !vm["dont-runtests"].as<bool>();
  }

  bool quitAfterTests = false;
  if(runTests && vm.count("quit-after-tests")) {
    quitAfterTests = vm["quit-after-tests"].as<bool>();
  }

  if(vm.count("prop")) {
      properties = vm["prop"].as< vector<string> >();
  }

  string snifferDev;
  bool startSniffer = false;

  if(vm.count("sniff")) {
    startSniffer = true;
    snifferDev = vm["sniff"].as<string>();
  }

#ifdef WITH_TESTS
  cout << "compiled WITH_TESTS" << endl;
  if(runTests) {
    cout << "running tests" << endl;
    dss::Tests::run();
    cout << "done running tests" << endl;
  }
#endif

  if(!quitAfterTests && startSniffer) {
#ifndef __APPLE__
    dss::DS485FrameSniffer sniffer(snifferDev);
    sniffer.run();
    while(true) {
      dss::sleepSeconds(10);
    }
#endif
  } else {
    if(!quitAfterTests) {
      // start DSS
      dss::DSS::getInstance()->initialize(properties);
      dss::DSS::getInstance()->run();
    }
    if(dss::DSS::hasInstance()) {
      dss::DSS::shutdown();
    }
    dss::Logger::shutdown();
  }

#ifdef USE_LIBXML
  // free some internal memory of libxml to make valgrind happy
  // NOTE: this needs to be the last call in a process.
  //       Trust me....
  xmlCleanupParser();
#endif
  free(tzNameCopy);

  return 0;
}
