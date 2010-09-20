/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland

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

#ifndef SETBUILDER_H_
#define SETBUILDER_H_

#include <string>

#include <boost/shared_ptr.hpp>

#include "core/ds485types.h"

namespace dss {

  class Zone;
  class Apartment;
  class Set;
  
  class SetBuilder {
  private:
    std::string m_SetDescription;
    Apartment& m_Apartment;
  protected:
    Set restrictBy(const std::string& _identifier, const Set& _set, boost::shared_ptr<const Zone> _zone);
    Set restrictByFunction(const std::string& _functionName, unsigned int& _index, const Set& _set, boost::shared_ptr<const Zone> _zone);
    void skipWhitespace(unsigned int& _index);
    std::string readParameter(unsigned int& _index);
    int readInt(unsigned int& _index);
    dsid_t readDSID(unsigned int& _index);
    std::string readString(unsigned int& _index);
    Set parseSet(unsigned int& _index, const Set& _set, boost::shared_ptr<const Zone> _context);
  public:
    SetBuilder(Apartment& _apartment);

    Set buildSet(const std::string& _setDescription, boost::shared_ptr<const Zone> _context);
  }; // SetBuilder

} // namespace dss

#endif /* SETBUILDER_H_ */
