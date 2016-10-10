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
#include "vdc-token.h"
#include "dss.h"
#include "base.h"
#include "datetools.h"
#include "sessionmanager.h"
#include "webservice_api.h"
#include "security/security.h"

typedef boost::system::error_code error_code;

namespace dss {

__DEFINE_LOG_CHANNEL__(VdcToken, lsInfo);

VdcToken::VdcToken(DSS &dss) :
    m_dss(dss),
    m_timer(dss.getIoService()),
    m_enabled(false),
    m_configNode(dss.getPropertySystem().createProperty("/config/vdcToken")),
    m_expirePeriod(boost::chrono::seconds(m_configNode->getOrCreateIntChild(
        "expirePeriodSeconds", 30 * 24 * 60 * 60))),
    m_retryPeriod(boost::chrono::seconds(m_configNode->getOrCreateIntChild(
        "retryPeriodSeconds", 300))) {

  assert(m_expirePeriod.count() != 0);
  assert(m_retryPeriod.count() != 0);
  log("VdcToken m_expirePeriod:" + intToString(m_expirePeriod.count())
      + " m_retryPeriod:" + intToString(m_retryPeriod.count()), lsNotice);
  auto& propertySystem = m_dss.getPropertySystem();

  m_enabledNode = propertySystem.getProperty(pp_websvc_mshub_active);
  m_enabledNode->addListener(this);

  const auto& securityNode = m_dss.getSecurity().getRootNode();
  // security property tree is persistent and it is loaded at this point

  auto node = securityNode->createProperty("vdcToken");
  m_tokenNode = node->createProperty("value");
  m_tokenNode->setFlag(PropertyNode::Archive, true);
  m_issuedOnNode = node->createProperty("issuedOn");
  m_issuedOnNode->setFlag(PropertyNode::Archive, true);

  asyncRestart();
}

bool VdcToken::isExpired() const {
  try {
    auto dateTime = DateTime::parseISO8601(m_issuedOnNode->getStringValue());
    dateTime = dateTime.addSeconds(boost::chrono::duration_cast<boost::chrono::seconds>(m_expirePeriod).count());
    if (dateTime > DateTime()) {
      return false;
    }
  } catch (std::exception &e) {} // consider invalid issuedOn date to be expired
  return true;
}

void VdcToken::propertyChanged(PropertyNodePtr caller, PropertyNodePtr node) {
  // ATTENTION: may run in different thread
  m_dss.getIoService().dispatch([this, node]() {
    if (node == m_enabledNode) {
      auto enabled = m_enabledNode->getBoolValue();
      if (m_enabled == enabled) {
        return;
      }
      m_enabled = enabled;
      log("m_enabled:" + intToString(m_enabled), lsInfo);
      if (!m_enabled) {
        // token is invalid if remote connectivity is disabled
        revokeToken();
      }
      asyncRestart();
    }
  });
}

// restart the asynchronous loop.
void VdcToken::asyncRestart() {
  m_timer.cancel();
  if (!m_enabled) {
    log("!m_enabled -> stop", lsDebug);
    return;
  }
  asyncLoop();
}

void VdcToken::asyncLoop() {
  m_dss.assertIoServiceThread();
  assert(m_enabled);
  auto isExpired = this->isExpired();
  boost::chrono::seconds timeout;

  if (isExpired) {
    // short timeout to retry push. We restart asyncLoop after successful push
    log("Token is expired, pushing new one.", lsNotice);
    timeout = m_retryPeriod;
    try {
      asyncPush();
    } catch (std::exception &e) {
      //make sure to not stop the loop
      log(std::string("asyncPush failed e.what():") + e.what(), lsError);
    }
  } else {
    // Long timeout to push again when token expires.
    // We are lazy to calculate number of seconds till expiration.
    // It would be wrong anyway if system time changes in meantime.
    // We consider expiration only as a hint when to push new token.
    log("Token is valid.", lsDebug);
    timeout = m_expirePeriod / 10;
  }
  m_timer.expires_from_now(timeout);
  m_timer.async_wait([=](const error_code &e) {
    if (e) {
      return; // aborted
    }
    asyncLoop();
  });
}

// generate new token, upload to msHub. On success store token locally and restart loop
void VdcToken::asyncPush() {
  std::string token = SessionTokenGenerator::generate();
  WebserviceMsHub::doVdcStoreVdcToken(token, WebserviceCallDone_t(new WebserviceCallDoneFunction(
      [this, token](RestTransferStatus_t status, const WebserviceReply& reply) {
        // ATTENTION: may run in different thread
        if (status != REST_OK && reply.code != 200) {
          log("upload failed. status:" + intToString(status)
              + " code:" + intToString(reply.code) + " message:" + reply.desc, lsError);
          // we will try again on next push retry timer tick
          return;
        }
        m_dss.getIoService().dispatch([this, token]() { onPushed(token); });
      })));
  // The doVdcStoreVdcToken processing is not cancelled in asyncRestart.
  // So the requests can queue in the worker thread if they take longer
  // to finish than ASYNC_RETRY_TIMEOUT.
  // The doVdcStoreVdcToken synchronous operation does not return any
  // handle that can be used to cancel it :(.
  // It is possible to block further doVdcStoreVdcToken calls until we get reply from previous one.
  // But that is also risky, because we rely on the callback being called.
  // It is not really the practice in WebserviceMsHub code.
}

void VdcToken::onPushed(const std::string& token) {
  revokeToken();

  auto& security = m_dss.getSecurity();
  security.createApplicationToken("vdc", token);
  security.enableToken(token);
  m_tokenNode->setStringValue(token);
  m_issuedOnNode->setStringValue(DateTime().toISO8601());
  log("New token uploaded and stored. token:" + token, lsNotice);
  asyncLoop();
}

void VdcToken::revokeToken() {
  auto oldToken = m_tokenNode->getAsString();
  m_tokenNode->setStringValue(std::string());
  m_issuedOnNode->setStringValue(std::string());
  if (!oldToken.empty()) {
    log("revoked existing token. oldToken:" + oldToken, lsInfo);
    m_dss.getSecurity().revokeToken(oldToken);
  }
}

} // namespace dss
