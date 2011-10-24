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

#include "core/foreach.h"
#include "addressablemodelitem.h"

namespace dss {

  //================================================== NonAddressableModelItem

  void NonAddressableModelItem::increaseValue() {
    std::vector<boost::shared_ptr<AddressableModelItem> > items = splitIntoAddressableItems();
    foreach(boost::shared_ptr<AddressableModelItem> item, items) {
      item->increaseValue();
    }
  } // increaseValue

  void NonAddressableModelItem::decreaseValue() {
    std::vector<boost::shared_ptr<AddressableModelItem> > items = splitIntoAddressableItems();
    foreach(boost::shared_ptr<AddressableModelItem> item, items) {
      item->decreaseValue();
    }
  } // decreaseValue

  void NonAddressableModelItem::setValue(const uint8_t _value) {
    std::vector<boost::shared_ptr<AddressableModelItem> > items = splitIntoAddressableItems();
    foreach(boost::shared_ptr<AddressableModelItem> item, items) {
      item->setValue(_value);
    }
  } // setValue

  void NonAddressableModelItem::callScene(const int _sceneNr, const bool _force) {
    std::vector<boost::shared_ptr<AddressableModelItem> > items = splitIntoAddressableItems();
    foreach(boost::shared_ptr<AddressableModelItem> item, items) {
      item->callScene(_sceneNr, _force);
    }
  } // callScene

  void NonAddressableModelItem::saveScene(const int _sceneNr) {
    std::vector<boost::shared_ptr<AddressableModelItem> > items = splitIntoAddressableItems();
    foreach(boost::shared_ptr<AddressableModelItem> item, items) {
      item->saveScene(_sceneNr);
    }
  } // saveScene

  void NonAddressableModelItem::undoScene(const int _sceneNr) {
    std::vector<boost::shared_ptr<AddressableModelItem> > items = splitIntoAddressableItems();
    foreach(boost::shared_ptr<AddressableModelItem> item, items) {
      item->undoScene(_sceneNr);
    }
  } // undoScene

  void NonAddressableModelItem::undoSceneLast() {
    std::vector<boost::shared_ptr<AddressableModelItem> > items = splitIntoAddressableItems();
    foreach(boost::shared_ptr<AddressableModelItem> item, items) {
      item->undoSceneLast();
    }
  } // undoSceneLast

  void NonAddressableModelItem::blink() {
    std::vector<boost::shared_ptr<AddressableModelItem> > items = splitIntoAddressableItems();
    foreach(boost::shared_ptr<AddressableModelItem> item, items) {
      item->blink();
    }
  }

} // namespace dss
