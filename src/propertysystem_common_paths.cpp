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

#include "propertysystem_common_paths.h"

#include "config.h"

#ifdef HAVE_BUILD_INFO_H
  #include "build_info.h"
#endif

#include <sys/resource.h>

#include "dss.h"
#include "propertysystem.h"

namespace dss {

const char *pp_sysinfo_dsid = "/system/dSID";
const char *pp_sysinfo_dss_version = "/system/version/version";
const char *pp_sysinfo_distro_version = "/system/version/distroVersion";
#ifdef HAVE_BUILD_INFO_H
const char *pp_sysinfo_build_host = "/system/version/buildHost";
const char *pp_sysinfo_git_revision = "/system/version/gitRevision";
#endif

void setupCommonProperties(PropertySystem &propSystem) {
  propSystem.createProperty(pp_sysinfo_dss_version)->setStringValue(DSS_VERSION);
  propSystem.createProperty(pp_sysinfo_distro_version)
    ->setStringValue(DSS::readDistroVersion());
#ifdef HAVE_BUILD_INFO_H
  propSystem.createProperty(pp_sysinfo_build_host)->setStringValue(DSS_BUILD_HOST);
  propSystem.createProperty(pp_sysinfo_git_revision)->setStringValue(DSS_RCS_REVISION);
#endif

  // TODO if adding predifined strings, also use them in main.cpp
  propSystem.createProperty("/config/debug/coredumps/enabled")->setBooleanValue(false);
  propSystem.createProperty("/config/debug/coredumps/limit")->setFloatingValue(RLIM_INFINITY);
}

}
