/*
 * seriespersistence.h
 *
 *  Created on: Jan 30, 2009
 *      Author: patrick
 */

#ifndef SERIESPERSISTENCE_H_
#define SERIESPERSISTENCE_H_

#include "series.h"

namespace dss {

  template<class T>
  class SeriesReader {
  public:
    Series<T>* readFromXML(const string& _fileName);
  }; // SeriesReader

  template<class T>
  class SeriesWriter {
  public:
    bool writeToXML(const Series<T>& _series, const string& _path);
  }; // SeriesWriter
} // namespace dss

#endif /* SERIESPERSISTENCE_H_ */
