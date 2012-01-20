/*
    Copyright (c) 2012 digitalSTROM.org, Zurich, Switzerland

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

#ifndef _JSCURL_INCLUDED
#define _JSCURL_INCLUDED

#include <curl/curl.h>
#include "src/scripting/jshandler.h"

namespace dss {

/** This class extends a ScriptContext with a libcurl interface */
class CurlScriptContextExtension : public ScriptExtension {
public:
  /** Creates a libcurl wrapper instance */
  CurlScriptContextExtension();
  virtual ~CurlScriptContextExtension() {}

  virtual void extendContext(ScriptContext& _context);
};

}

#endif
