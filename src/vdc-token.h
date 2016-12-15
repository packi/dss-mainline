/*
    Copyright (c) 2009,2010 digitalSTROM.org, Zurich, Switzerland

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
#pragma once

#include "propertysystem.h"
#include <ds/asio/timer.h>

namespace dss {

class DSS;

/// State machine uploading vdc-token to mshub clound and updating it periodically.
///
/// Vdc-token is stored on both dss and mshub cloud. Local UI / mobile-app UI receives
/// vdc-token from DSS / ms-hub and passes it to vdc cloud UI to authenticate
/// its requests to mshub and dss.
/// Design : https://sharepoint.newtechgroup.ch/aizo/projects/A16050_vDC%20Environment/_layouts/15/WopiFrame2.aspx?sourcedoc=/aizo/projects/A16050_vDC%20Environment/Shared%20Documents/ProjectDocumentation/A16050_CloudSolution.pptx
///
/// We could remove this functionality from dss and keep the changing vdc-token recompletely in mshub.
/// Mshub could even generate dirrent token for each UI session.
/// Dss UI would follow the same path as mobile app UI. It receives the vdc-token from ms-hub.
/// Mshub would then translate vdc-token to existing osp-token when passing requests to dss.
class VdcToken : private PropertyListener {
public:
  VdcToken(DSS &dss);

private:
  __DECL_LOG_CHANNEL__;
  bool isExpired() const;

  void propertyChanged(PropertyNodePtr caller, PropertyNodePtr node) /* override */;

  void asyncRestart();
  void asyncLoop();
  void asyncPush();
  void onPushed(const std::string& token);
  void revokeToken();

private:
  DSS& m_dss;
  ds::asio::Timer m_timer;
  bool m_enabled; //thread local copy of m_enabledNode value
  PropertyNodePtr m_configNode;
  PropertyNodePtr m_enabledNode;
  PropertyNodePtr m_tokenNode;
  PropertyNodePtr m_issuedOnNode;
  boost::chrono::seconds m_expirePeriod;
  boost::chrono::seconds m_retryPeriod;
};

} // namespace dss
