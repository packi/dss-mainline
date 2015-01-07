/*
    Copyright (c) 2009, 2010 digitalSTROM.org, Zurich, Switzerland

    Author: Patrick Staehlin, futureLAB AG <pstaehlin@futurelab.ch>

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

*/


#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "src/base.h"
#include "src/dss.h"
#include "src/logger.h"
#include "src/datetools.h"
#include "src/backtrace.h"

#include <ctime>
#include <csignal>
#include <getopt.h>
#ifdef HAVE_SYS_PRCTL_H
#include <sys/prctl.h>
#endif
#include <sys/resource.h>
#include <string.h>

#include <boost/program_options.hpp>

#ifdef USE_LIBXML
  #include <libxml/tree.h>
  #include <libxml/encoding.h>
#endif

#include <iostream>

using namespace std;
namespace po = boost::program_options;

pair<string, string> parse_prop(const string& s) {
    if ((s.find("--prop") == 0) && (s.length() >=7)) {
      return make_pair("prop", s.substr(7));
    } else {
      return make_pair(string(), string());
    }
} // parse_prop

void dssHandleSignal(int _sig) {
  switch (_sig) {
  case SIGINT:
    fprintf(stderr, "\nSystem signal SIGINT\n");
    break;
  }
}

bool platformSpecificStartup() {
#ifndef WIN32
  srand((getpid() << 16) ^ getuid() ^ time(0));
  sigset_t signal_set;
  pthread_t sig_thread;

#ifdef HAVE_PRCTL
  // disable coredumps
  prctl(PR_SET_DUMPABLE, 0, 0, 0, 0);
#endif

  /* block all signals, enable and handle only a few selected signals globally */
  sigfillset(&signal_set);
  sigdelset(&signal_set, SIGINT);
  sigdelset(&signal_set, SIGQUIT);
  sigdelset(&signal_set, SIGSEGV);
  sigdelset(&signal_set, SIGABRT);
  sigdelset(&signal_set, SIGUSR2); /* gperftools dump profile */
  sigdelset(&signal_set, SIGPROF); /* gperftools cpu profiler trigger */
  pthread_sigmask(SIG_BLOCK, &signal_set, NULL);

  /* create the signal handling thread */
  return (0 == pthread_create(&sig_thread, NULL, dss::DSS::handleSignal, NULL));
#else
  srand( (int)time( (time_t)NULL ) );
  WSAData dat;
  WSAStartup( 0x1010, &dat );
#endif
} // platformSpecificStartup

char* tzNameCopy;

bool setupLocale() {
  if (!setlocale(LC_CTYPE, "")) {
    dss::Logger::getInstance()->log("Can't set the specified locale! Check LANG, LC_CTYPE, LC_ALL.", lsError);
    return false;
  }

  return true;
}

int main (int argc, char* argv[]) {

  if(!setupLocale()) {
    return -1;
  }
  if (!platformSpecificStartup()) {
    return -1;
  }
  dss::init_libraries();

  vector<string> properties;

  po::options_description desc("Allowed options");
  desc.add_options()
      ("help", "produce help message")
      ("prop", po::value<vector<string> >(), "sets a property")
      ("version,v", "print version information and exit")
      ("datadir,D", po::value<string>(), "set data directory")
      ("webrootdir,w", po::value<string>(), "set webroot directory")
      ("configdir,c", po::value<string>(), "set config directory")
      ("config,C", po::value<string>(), "set configuration file to use")
      ("savedpropsdir,p", po::value<string>(), "set saved properties directory")
#ifdef HAVE_PRCTL
      ("coredumps", po::value<string>(), "set to true to enable coredumps")
      ("corelimit", po::value<string>(), "maximum allowed coredump size")
#endif
#ifndef __APPLE__ // daemon() is marked as deprecated on OS X
      ("daemonize,d", "start as a daemon")
#endif
  ;

  po::variables_map vm;
  try {
    po::store(po::command_line_parser(argc, argv).options(desc).extra_parser(parse_prop)
          .run(), vm);
  } catch (const po::error& e) {
    cout << "Error parsing command line: " << e.what() << endl;
    return 1;
  }
  po::notify(vm);

  if (vm.count("help")) {
      cout << desc << "\n";
      return 1;
  }

  if (vm.count("version")) {
    std::cout << dss::DSS::versionString() << std::endl;
    return 1;
  }

  if(vm.count("prop")) {
      properties = vm["prop"].as< vector<string> >();
  }

  if(vm.count("datadir")) {
    properties.push_back("/config/datadirectory=" +
                         vm["datadir"].as<string>());
  }

  if(vm.count("webrootdir")) {
    properties.push_back("/config/webrootdirectory=" +
                          vm["webrootdir"].as<string>());
  }

  if(vm.count("configdir")) {
    properties.push_back("/config/configdirectory=" +
                         vm["configdir"].as<string>());
  }

  string config_file;
  if(vm.count("config")) {
    config_file = vm["config"].as<string>();
  }

  if(vm.count("savedpropsdir")) {
    properties.push_back("/config/savedpropsdirectory=" +
                         vm["savedpropsdir"].as<string>());
  }

#ifdef HAVE_PRCTL
  if (vm.count("coredumps")) {
    properties.push_back("/config/debug/coredumps/enabled=" +
                         vm["coredumps"].as<string>());
    if (vm["coredumps"].as<string>() == "true") {
      if (prctl(PR_SET_DUMPABLE, 1, 0, 0, 0) == -1) {
        fprintf(stderr, "Could not apply coredump settings: %s\n",
                        strerror(errno));
      }
      struct rlimit rlim;
      rlim.rlim_cur = RLIM_INFINITY;
      rlim.rlim_max = RLIM_INFINITY;
      if (vm.count("corelimit")) {
        try {
          rlim.rlim_max = dss::strToUInt(vm["corelimit"].as<string>());
          rlim.rlim_cur = rlim.rlim_max;

          properties.push_back("/config/debug/coredumps/limit=" +
                         vm["corelimit"].as<string>());
        } catch(std::invalid_argument&) {
          fprintf(stderr, "Could not parse softlimit value: %s\n",
                           vm["corelimit"].as<string>().c_str());
          exit(EXIT_FAILURE);
        }
      }
      if (setrlimit(RLIMIT_CORE, &rlim) != 0) {
        fprintf(stderr, "Could not configure coredump size limit: %s\n",
                       strerror(errno));
        exit(EXIT_FAILURE);
      }
    }
  }
#endif


#ifndef __APPLE__
  bool daemonize = vm.count("daemonize") != 0;
#endif

  // start DSS
  if (dss::DSS::getInstance()->initialize(properties, config_file)) {
#ifndef __APPLE__
    if(daemonize) {
      int result = daemon(1,0);
      if(result != 0) {
        perror("daemon()");
        return 0;
      }
    }
#endif
    dss::DSS::getInstance()->run();
  } else {
    dss::Logger::getInstance()->log("Failed to initialize dss. Exiting", lsFatal);
  }
  if(dss::DSS::hasInstance()) {
    dss::DSS::shutdown();
  }
  dss::Logger::shutdown();
  dss::cleanup_libraries();

  free(tzNameCopy);

  return 0;
}
