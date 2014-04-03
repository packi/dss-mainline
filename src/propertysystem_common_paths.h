/*
    Copyright (c) 2014 digitalSTROM AG, Zurich, Switzerland

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

#ifndef __PROPERTYSYSTEM_COMMON_PATHS__
#define __PROPERTYSYSTEM_COMMON_PATHS__

#include "dssfwd.h"

namespace dss {

extern const char *pp_sysinfo_dss_version;
extern const char *pp_sysinfo_distro_version;
#ifdef HAVE_BUILD_INFO_H
extern const char *pp_sysinfo_build_host;
extern const char *pp_sysinfo_git_revision;
#endif

void setupCommonProperties(PropertySystem &propSystem);

}

#endif
