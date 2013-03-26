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


#ifndef ADDRESSABLEMODELITEM_H
#define ADDRESSABLEMODELITEM_H

#include <boost/enable_shared_from_this.hpp>

#include "physicalmodelitem.h"
#include "deviceinterface.h"

namespace dss {

  class Apartment;
  class PropertyNode;
  typedef boost::shared_ptr<PropertyNode> PropertyNodePtr;

  class AddressableModelItem : public PhysicalModelItem,
                               public IDeviceInterface,
                               public boost::enable_shared_from_this<AddressableModelItem> {
  public:
    AddressableModelItem(Apartment* _pApartment);

    virtual void increaseValue(const callOrigin_t _origin, const SceneAccessCategory _category);
    virtual void decreaseValue(const callOrigin_t _origin, const SceneAccessCategory _category);

    virtual void setValue(const callOrigin_t _origin, const SceneAccessCategory _category, const uint8_t _value);

    virtual void callScene(const callOrigin_t _origin, const SceneAccessCategory _category, const int _sceneNr, const bool _force);
    virtual void saveScene(const callOrigin_t _origin, const int _sceneNr);
    virtual void undoScene(const callOrigin_t _origin, const SceneAccessCategory _category, const int _sceneNr);
    virtual void undoSceneLast(const callOrigin_t _origin, const SceneAccessCategory _category);

    virtual void blink(const callOrigin_t _origin, const SceneAccessCategory _category);
  protected:
    Apartment* m_pApartment;
    PropertyNodePtr m_pPropertyNode;
  }; // AddressableModelItem

}

#endif // ADDRESSABLEMODELITEM_H
