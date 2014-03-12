/*
    Copyright (c) 2010 digitalSTROM.org, Zurich, Switzerland

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

#include "addressablemodelitem.h"

#include <boost/shared_ptr.hpp>

#include "src/businterface.h"
#include "src/propertysystem.h"
#include "src/model/apartment.h"
#include "src/model/modelconst.h"

namespace dss {

  //================================================== AddressableModelItem

  AddressableModelItem::AddressableModelItem(Apartment* _pApartment)
  : m_pApartment(_pApartment)
  {} // ctor

  void AddressableModelItem::increaseValue(const callOrigin_t _origin, const SceneAccessCategory _category) {
    if(m_pPropertyNode) {
      m_pPropertyNode->checkWriteAccess();
    }
    if (!SceneAccess::checkAccess(this, _category)) {
      Logger::getInstance()->log("AddressableModelItem: increaseValue blocked", lsDebug);
      return;
    }
    m_pApartment->getActionRequestInterface()->callScene(this, _origin, _category, SceneInc, "", false);
  } // increaseValue

  void AddressableModelItem::decreaseValue(const callOrigin_t _origin, const SceneAccessCategory _category) {
    if(m_pPropertyNode) {
      m_pPropertyNode->checkWriteAccess();
    }
    if (!SceneAccess::checkAccess(this, _category)) {
      Logger::getInstance()->log("AddressableModelItem: decreaseValue blocked", lsDebug);
      return;
    }
    m_pApartment->getActionRequestInterface()->callScene(this, _origin, _category, SceneDec, "", false);
  } // decreaseValue

  void AddressableModelItem::setValue(const callOrigin_t _origin, const SceneAccessCategory _category, const uint8_t _value, const std::string _token) {
    if(m_pPropertyNode) {
      m_pPropertyNode->checkWriteAccess();
    }
    if (!SceneAccess::checkAccess(this, _category)) {
      Logger::getInstance()->log("AddressableModelItem: setValue blocked", lsDebug);
      return;
    }
    m_pApartment->getActionRequestInterface()->setValue(this, _origin, _category, _value, _token);
  } // setValue

  void AddressableModelItem::callScene(const callOrigin_t _origin, const SceneAccessCategory _category, const int _sceneNr, const std::string _token, const bool _force) {
    if(m_pPropertyNode) {
      m_pPropertyNode->checkWriteAccess();
    }
    if (!SceneAccess::checkAccess(this, _category)) {
      Logger::getInstance()->log("AddressableModelItem: callScene blocked", lsDebug);
      return;
    }
    if ((_origin == coJSON || _origin == coSOAP)) {
      if (_force == false && (!SceneAccess::checkStates(this, _sceneNr))) {
        Logger::getInstance()->log("AddressableModelItem: callScene from web service blocked", lsDebug);
        return;
      }
    }
    m_pApartment->getActionRequestInterface()->callScene(this, _origin, _category, _sceneNr, _token, _force);
  } // callScene

  void AddressableModelItem::saveScene(const callOrigin_t _origin, const int _sceneNr, const std::string _token) {
    if(m_pPropertyNode) {
      m_pPropertyNode->checkWriteAccess();
    }
    m_pApartment->getActionRequestInterface()->saveScene(this, _origin, _sceneNr, _token);
  } // saveScene

  void AddressableModelItem::undoScene(const callOrigin_t _origin, const SceneAccessCategory _category, const int _sceneNr, const std::string _token) {
    if(m_pPropertyNode) {
      m_pPropertyNode->checkWriteAccess();
    }
    if (!SceneAccess::checkAccess(this, _category)) {
      Logger::getInstance()->log("AddressableModelItem: undoScene blocked", lsDebug);
      return;
    }
    m_pApartment->getActionRequestInterface()->undoScene(this, _origin, _category, _sceneNr, _token);
  } // undoScene

  void AddressableModelItem::undoSceneLast(const callOrigin_t _origin, const SceneAccessCategory _category, const std::string _token) {
    if(m_pPropertyNode) {
      m_pPropertyNode->checkWriteAccess();
    }
    if (!SceneAccess::checkAccess(this, _category)) {
      Logger::getInstance()->log("AddressableModelItem: undoSceneLast blocked", lsDebug);
      return;
    }
    m_pApartment->getActionRequestInterface()->undoSceneLast(this, _origin, _category, _token);
  } // undoSceneLast

  void AddressableModelItem::blink(const callOrigin_t _origin, const SceneAccessCategory _category, const std::string _token) {
    if(m_pPropertyNode) {
      m_pPropertyNode->checkWriteAccess();
    }
    if (!SceneAccess::checkAccess(this, _category)) {
      Logger::getInstance()->log("AddressableModelItem: blink blocked", lsDebug);
      return;
    }
    m_pApartment->getActionRequestInterface()->blink(this, _origin, _category, _token);
  } // blink

  void AddressableModelItem::increaseOutputChannelValue(const callOrigin_t _origin, const SceneAccessCategory _category, const uint8_t _channel, const std::string _token) {
    if(m_pPropertyNode) {
      m_pPropertyNode->checkWriteAccess();
    }
    if (!SceneAccess::checkAccess(this, _category)) {
      Logger::getInstance()->log("AddressableModelItem: increaseOutputChannelValue blocked", lsDebug);
      return;
    }
    m_pApartment->getActionRequestInterface()->increaseOutputChannelValue(this, _origin, _category, _channel, _token);
  } // increaseOutputChannelValue

  void AddressableModelItem::decreaseOutputChannelValue(const callOrigin_t _origin, const SceneAccessCategory _category, const uint8_t _channel, const std::string _token) {
    if(m_pPropertyNode) {
      m_pPropertyNode->checkWriteAccess();
    }
    if (!SceneAccess::checkAccess(this, _category)) {
      Logger::getInstance()->log("AddressableModelItem: decreaseOutputChannelValue blocked", lsDebug);
      return;
    }
    m_pApartment->getActionRequestInterface()->decreaseOutputChannelValue(this, _origin, _category, _channel, _token);
  } // decreaseOutputChannelValue

  void AddressableModelItem::stopOutputChannelValue(const callOrigin_t _origin, const SceneAccessCategory _category, const uint8_t _channel, const std::string _token) {
    if(m_pPropertyNode) {
      m_pPropertyNode->checkWriteAccess();
    }
    if (!SceneAccess::checkAccess(this, _category)) {
      Logger::getInstance()->log("AddressableModelItem: stopOutputChannelValue blocked", lsDebug);
      return;
    }
    m_pApartment->getActionRequestInterface()->stopOutputChannelValue(this, _origin, _category, _channel, _token);
  } // stopOutputChannelValue
} // namespace dss
