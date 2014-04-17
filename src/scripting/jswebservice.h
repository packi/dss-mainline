/*
    Copyright (c) 2014 digitalSTROM AG, Zurich, Switzerland

    Author: Sergey Bostandzhyan <jin@dev.digitalstrom.org>

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

#ifndef __DSS_JSWEBSERVICE_H__
#define __DSS_JSWEBSERVICE_H__

namespace dss {

class WebserviceConnectionScriptContextExtension : public ScriptExtension {
public:
  WebserviceConnectionScriptContextExtension();
  virtual ~WebserviceConnectionScriptContextExtension() {}

  virtual void extendContext(ScriptContext& _context);
};

} // namespace

#endif //__DSS_JSWEBSERVICE_H__
