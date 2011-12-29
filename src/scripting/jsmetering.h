/*
    Copyright (c) 2011 digitalSTROM.org, Zurich, Switzerland

    Author: Michael Tross, aizo GmbH <michael.tross@aizo.com>

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

#ifndef _JSMETERING_INCLUDED
#define _JSMETERING_INCLUDED

#include <boost/ptr_container/ptr_vector.hpp>
#include "src/scripting/jshandler.h"
#include "src/event.h"
#include "src/propertysystem.h"

namespace dss {

  class Apartment;
  class Metering;

  class MeteringScriptExtension : public ScriptExtension {
  public:
    MeteringScriptExtension(Apartment& _apartment, Metering& _metering);
    virtual ~MeteringScriptExtension() {}

    virtual void extendContext(ScriptContext& _context);

    Apartment& getApartment() { return m_Apartment; }
    Metering& getMetering() { return m_Metering; }
  private:
    Apartment& m_Apartment;
    Metering& m_Metering;
  }; // MeteringScriptExtension

}

#endif
