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
#ifndef NONADDRESSABLEMODELITEM_H
#define NONADDRESSABLEMODELITEM_H

#include <vector>

#include <boost/shared_ptr.hpp>

#include "devicereference.h"

namespace dss {

  class AddressableModelItem;

  class NonAddressableModelItem : public IDeviceInterface {
  public:
    virtual void increaseValue(const callOrigin_t _origin, const SceneAccessCategory _category);
    virtual void decreaseValue(const callOrigin_t _origin, const SceneAccessCategory _category);

    virtual void setValue(const callOrigin_t _origin, const SceneAccessCategory _category, const uint8_t _value, const std::string _token);

    virtual void callScene(const callOrigin_t _origin, const SceneAccessCategory _category, const int _sceneNr, std::string _token, const bool _force = false);
    virtual void callSceneMin(const callOrigin_t _origin, const SceneAccessCategory _category, const int _sceneNr, const std::string _token);
    virtual void saveScene(const callOrigin_t _origin, const int _sceneNr, const std::string _token);
    virtual void undoScene(const callOrigin_t _origin, const SceneAccessCategory _category, const int _sceneNr, const std::string _token);
    virtual void undoSceneLast(const callOrigin_t _origin, const SceneAccessCategory _category, const std::string _token);

    virtual void blink(const callOrigin_t _origin, const SceneAccessCategory _category, const std::string _token);
    virtual void increaseOutputChannelValue(const callOrigin_t _origin, const SceneAccessCategory _category, const uint8_t channel, const std::string _token);
    virtual void decreaseOutputChannelValue(const callOrigin_t _origin, const SceneAccessCategory _category, const uint8_t channel, const std::string _token);
    virtual void stopOutputChannelValue(const callOrigin_t _origin, const SceneAccessCategory _category, const uint8_t channel, const std::string _token);

    virtual void pushSensor(const callOrigin_t _origin, const SceneAccessCategory _category, const dsuid_t _sourceID, const uint8_t _sensorType, const double _sensorValueFloat, const std::string _token);
  protected:
    virtual std::vector<boost::shared_ptr<AddressableModelItem> > splitIntoAddressableItems() = 0;
  }; // NonAddressableModelItem

} // namespace dss

#endif // NONADDRESSABLEMODELITEM_H
