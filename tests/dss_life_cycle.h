/*
    Copyright (c) 2014 digitalSTROM.org, Zurich, Switzerland

    Author: Andreas Fenkart

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

#ifndef __DSS_LIFE_CYCLE__
#define __DSS_LIFE_CYCLE__

#include "dss.h"

namespace dss {

/**
 * DSSLifeCycle - RAII like management of DSS instance
 *
 * why does it need to be FRIEND of DSS?
 *
 * boost::scoped_ptr<DSSLifeCycle> guard;
 * guard.reset(new DSSLifeCycle());
 * guard.reset(new DSSLifeCycle());
 *
 * the second reset will fetch a refence of the old
 * instance then assign it to the guard, which already
 * holds a reference to the same instance. This reassignment
 * will trigger the deletion of the single instance
 *
 * Workaround:
 * - do DSS::shutdown before creating instance
 * - only destroy current instance if it is still equal
 *   to the one we created, requires access to m_Instance
 *
 * USE like this:
 * DSSLifeCycle guard;
 * std::scoped_ptr<DSSLifeCycle> guard
 */
class DSSLifeCycle {
  __DECL_LOG_CHANNEL__
public:
  DSSLifeCycle();
  ~DSSLifeCycle();
protected:
  DSS *m_instance;
  int m_incarnation;
  std::string m_configFileName;
};

}

#endif
