/*
    Copyright (c) 2010 digitalSTROM.org, Zurich, Switzerland
    Copyright (c) 2010 futureLAB AG, Winterthur, Switzerland

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
#ifndef NONADDRESSABLEMODELITEM_H
#define NONADDRESSABLEMODELITEM_H

#include <vector>

#include "devicereference.h"

namespace dss {

  class AddressableModelItem;
  
  class NonAddressableModelItem : public IDeviceInterface {
  public:
    virtual void increaseValue();
    virtual void decreaseValue();

    virtual void startDim(const bool _directionUp);
    virtual void endDim();

    virtual void setValue(const double _value);

    virtual void callScene(const int _sceneNr);
    virtual void saveScene(const int _sceneNr);
    virtual void undoScene(const int _sceneNr);
/*
    virtual void nextScene();
    virtual void previousScene();
*/
  protected:
    virtual std::vector<AddressableModelItem*> splitIntoAddressableItems() = 0;
  }; // NonAddressableModelItem

} // namespace dss

#endif // NONADDRESSABLEMODELITEM_H
