// g++ -o dsidhelper_test test.cpp ds485types.cpp base.cpp

#include <iostream>

#include "ds485types.h"
#include "dsidhelper.h"

using namespace dss;

int main(int argc, char** argv)
{
  dss_dsid_t dss_dsid = dsid::fromString("000102030405060708090a0b");
  

  dsid_t dsid;
  dsid_helper::toDsmapiDsid(dss_dsid, dsid);
  std::cout << dsid_helper::toString(dsid) << std::endl;
  
  dss_dsid_t dss_dsid2;
  dsid_helper::toDssDsid(dsid, dss_dsid2);
  std::cout << dss_dsid2.toString() << std::endl;
  
  return 0;
}
