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

#include "nonaddressablemodelitem.h"

#include "src/foreach.h"
#include "addressablemodelitem.h"

namespace dss {

  //================================================== NonAddressableModelItem

  void NonAddressableModelItem::increaseValue(const callOrigin_t _origin, const SceneAccessCategory _category) {
    std::vector<boost::shared_ptr<AddressableModelItem> > items = splitIntoAddressableItems();
    foreach(boost::shared_ptr<AddressableModelItem> item, items) {
      item->increaseValue(_origin, _category);
    }
  } // increaseValue

  void NonAddressableModelItem::decreaseValue(const callOrigin_t _origin, const SceneAccessCategory _category) {
    std::vector<boost::shared_ptr<AddressableModelItem> > items = splitIntoAddressableItems();
    foreach(boost::shared_ptr<AddressableModelItem> item, items) {
      item->decreaseValue(_origin, _category);
    }
  } // decreaseValue

  void NonAddressableModelItem::setValue(const callOrigin_t _origin, const uint8_t _value) {
    std::vector<boost::shared_ptr<AddressableModelItem> > items = splitIntoAddressableItems();
    foreach(boost::shared_ptr<AddressableModelItem> item, items) {
      item->setValue(_origin, _value);
    }
  } // setValue

  void NonAddressableModelItem::callScene(const callOrigin_t _origin, const SceneAccessCategory _category, const int _sceneNr, const bool _force) {
    std::vector<boost::shared_ptr<AddressableModelItem> > items = splitIntoAddressableItems();
    foreach(boost::shared_ptr<AddressableModelItem> item, items) {
      item->callScene(_origin, _category, _sceneNr, _force);
    }
  } // callScene

  void NonAddressableModelItem::saveScene(const callOrigin_t _origin, const int _sceneNr) {
    std::vector<boost::shared_ptr<AddressableModelItem> > items = splitIntoAddressableItems();
    foreach(boost::shared_ptr<AddressableModelItem> item, items) {
      item->saveScene(_origin, _sceneNr);
    }
  } // saveScene

  void NonAddressableModelItem::undoScene(const callOrigin_t _origin, const SceneAccessCategory _category, const int _sceneNr) {
    std::vector<boost::shared_ptr<AddressableModelItem> > items = splitIntoAddressableItems();
    foreach(boost::shared_ptr<AddressableModelItem> item, items) {
      item->undoScene(_origin, _category, _sceneNr);
    }
  } // undoScene

  void NonAddressableModelItem::undoSceneLast(const callOrigin_t _origin, const SceneAccessCategory _category) {
    std::vector<boost::shared_ptr<AddressableModelItem> > items = splitIntoAddressableItems();
    foreach(boost::shared_ptr<AddressableModelItem> item, items) {
      item->undoSceneLast(_origin, _category);
    }
  } // undoSceneLast

  void NonAddressableModelItem::blink(const callOrigin_t _origin, const SceneAccessCategory _category) {
    std::vector<boost::shared_ptr<AddressableModelItem> > items = splitIntoAddressableItems();
    foreach(boost::shared_ptr<AddressableModelItem> item, items) {
      item->blink(_origin, _category);
    }
  }

} // namespace dss
