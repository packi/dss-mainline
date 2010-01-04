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

#include "core/model/busrequest.h"
#include "core/model/apartment.h"

namespace dss {

  //================================================== AddressableModelItem

  AddressableModelItem::AddressableModelItem(Apartment* _pApartment)
  : m_pApartment(_pApartment)
  {} // ctor


  void AddressableModelItem::increaseValue() {
    boost::shared_ptr<IncreaseValueCommandBusRequest> request(new IncreaseValueCommandBusRequest());
    request->setTarget(this);
    m_pApartment->dispatchRequest(request);
  }

  void AddressableModelItem::decreaseValue() {
    boost::shared_ptr<DecreaseValueCommandBusRequest> request(new DecreaseValueCommandBusRequest());
    request->setTarget(this);
    m_pApartment->dispatchRequest(request);
  }

  void AddressableModelItem::startDim(const bool _directionUp) {
    boost::shared_ptr<CommandBusRequest> request;
    if(_directionUp) {
      request.reset(new StartDimUpCommandBusRequest());
    } else {
      request.reset(new StartDimDownCommandBusRequest());
    }
    request->setTarget(this);
    m_pApartment->dispatchRequest(request);
  } // startDim

  void AddressableModelItem::endDim() {
    boost::shared_ptr<CommandBusRequest> request(new EndDimCommandBusRequest());
    request->setTarget(this);
    m_pApartment->dispatchRequest(request);
  } // endDim

  void AddressableModelItem::setValue(const double _value) {
    boost::shared_ptr<SetValueCommandBusRequest> request(new SetValueCommandBusRequest());
    request->setTarget(this);
    request->setValue(int(_value));
    m_pApartment->dispatchRequest(request);
  } // setValue

  void AddressableModelItem::callScene(const int _sceneNr) {
    boost::shared_ptr<CallSceneCommandBusRequest> request(new CallSceneCommandBusRequest());
    request->setTarget(this);
    request->setSceneID(_sceneNr);
    m_pApartment->dispatchRequest(request);
  } // callScene

  void AddressableModelItem::saveScene(const int _sceneNr) {
    boost::shared_ptr<SaveSceneCommandBusRequest> request(new SaveSceneCommandBusRequest());
    request->setTarget(this);
    request->setSceneID(_sceneNr);
    m_pApartment->dispatchRequest(request);
  } // saveScene

  void AddressableModelItem::undoScene(const int _sceneNr) {
    boost::shared_ptr<UndoSceneCommandBusRequest> request(new UndoSceneCommandBusRequest());
    request->setTarget(this);
    request->setSceneID(_sceneNr);
    m_pApartment->dispatchRequest(request);
  } // undoScene
/*
  void nextScene();
  void previousScene();
*/
  
} // namespace dss
