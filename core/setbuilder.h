/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland
    Copyright (c) 2009 futureLAB AG, Winterthur, Switzerland
    Copyright (c) 2008 Patrick Staehlin <me@packi.ch>

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

#include "model.h"

#include <string>

using std::string;

namespace dss {
  class SetBuilder {
  private:
    std::string m_SetDescription;
  protected:
	  Set restrictBy(const string& _identifier, const Set& _set, const Zone& _zone);
	  Set restrictByFunction(const string& _functionName, unsigned int& _index, const Set& _set, const Zone& _zone);
	  void skipWhitespace(unsigned int& _index);
	  std::string readParameter(unsigned int& _index);
	  int readInt(unsigned int& _index);
	  dsid_t readDSID(unsigned int& _index);
	  Set parseSet(unsigned int& _index, const Set& _set, const Zone& _context);
	public:
	  SetBuilder();

	  Set buildSet(const string& _setDescription, const Zone* _context);
  }; // SetBuilder

} // namespace dss

#endif /* SETBUILDER_H_ */
