/*
 * restfulapiwriter.h
 *
 *  Created on: Jul 6, 2009
 *      Author: patrick
 */

#ifndef RESTFULAPIWRITER_H_
#define RESTFULAPIWRITER_H_

#include "restful.h"

#include <Poco/DOM/AutoPtr.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>

namespace dss {

  class RestfulAPIWriter {
  private:
    static Poco::XML::AutoPtr<Poco::XML::Element> writeToXML(const RestfulParameter& _parameter, Poco::XML::AutoPtr<Poco::XML::Document>& _document);
  public:
    static void writeToXML(const RestfulAPI& api, const std::string& _location);
  }; // RestfulAPIWriter

} // namespace dss

#endif /* RESTFULAPIWRITER_H_ */
