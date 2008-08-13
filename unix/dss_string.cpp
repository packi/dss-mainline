/*
 * dss_string.cpp
 *
 *  Created on: Aug 13, 2008
 *      Author: Patrick Stählin
 */

#include <string.h>

#include <string>

using namespace std;

namespace dss {
  int stricmp(const char* _s1, const char* _s2) {
	  string s1 = _s1;
	  string s2 = _s2;
	  std::transform(s1.begin(), s1.end(), s1.begin(), ::tolower);
	  std::transform(s2.begin(), s2.end(), s2.begin(), ::tolower);
	  return strcmp(s1.c_str(), s2.c_str());
  }
}
