/*
 * restfulapiwriter.h
 *
 *  Created on: Jul 6, 2009
 *      Author: patrick
 */

#ifndef RESTFULAPIWRITER_H_
#define RESTFULAPIWRITER_H_

#include "restful.h"

namespace dss {

  class RestfulAPIWriter {
  public:
    static void WriteToXML(const RestfulAPI& api, const std::string& _location);
  }; // RestfulAPIWriter

} // namespace dss

#endif /* RESTFULAPIWRITER_H_ */
