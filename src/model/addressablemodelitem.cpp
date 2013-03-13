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
    m_pApartment->getActionRequestInterface()->callScene(this, _origin, _category, SceneInc, false);
  } // increaseValue

  void AddressableModelItem::decreaseValue(const callOrigin_t _origin, const SceneAccessCategory _category) {
    if(m_pPropertyNode) {
      m_pPropertyNode->checkWriteAccess();
    }
    m_pApartment->getActionRequestInterface()->callScene(this, _origin, _category, SceneDec, false);
  } // decreaseValue

  void AddressableModelItem::setValue(const callOrigin_t _origin, const uint8_t _value) {
    if(m_pPropertyNode) {
      m_pPropertyNode->checkWriteAccess();
    }
    m_pApartment->getActionRequestInterface()->setValue(this, _origin, _value);
  } // setValue

  void AddressableModelItem::callScene(const callOrigin_t _origin, const SceneAccessCategory _category, const int _sceneNr, const bool _force) {
    if(m_pPropertyNode) {
      m_pPropertyNode->checkWriteAccess();
    }
    m_pApartment->getActionRequestInterface()->callScene(this, _origin, _category, _sceneNr, _force);
  } // callScene

  void AddressableModelItem::saveScene(const callOrigin_t _origin, const int _sceneNr) {
    if(m_pPropertyNode) {
      m_pPropertyNode->checkWriteAccess();
    }
    m_pApartment->getActionRequestInterface()->saveScene(this, _origin, _sceneNr);
  } // saveScene

  void AddressableModelItem::undoScene(const callOrigin_t _origin, const SceneAccessCategory _category, const int _sceneNr) {
    if(m_pPropertyNode) {
      m_pPropertyNode->checkWriteAccess();
    }
    m_pApartment->getActionRequestInterface()->undoScene(this, _origin, _category, _sceneNr);
  } // undoScene

  void AddressableModelItem::undoSceneLast(const callOrigin_t _origin, const SceneAccessCategory _category) {
    if(m_pPropertyNode) {
      m_pPropertyNode->checkWriteAccess();
    }
    m_pApartment->getActionRequestInterface()->undoSceneLast(this, _origin, _category);
  } // undoSceneLast

  void AddressableModelItem::blink(const callOrigin_t _origin, const SceneAccessCategory _category) {
    if(m_pPropertyNode) {
      m_pPropertyNode->checkWriteAccess();
    }
    m_pApartment->getActionRequestInterface()->blink(this, _origin, _category);
  } // blink

} // namespace dss
