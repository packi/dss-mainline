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
