/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland

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

#ifndef BUSINTERFACEHANDLER_H_
#define BUSINTERFACEHANDLER_H_

#include "core/thread.h"
#include "core/subsystem.h"
#include "core/syncevent.h"
#include "core/ds485/ds485.h"

namespace dss {

  class ModelMaintenance;
  class FrameBucketBase;
  class ModelEvent;

  typedef std::vector<boost::shared_ptr<DS485CommandFrame> > CommandFrameSharedPtrVector;

  class BusInterfaceHandler : public Thread,
                              public Subsystem,
                              public IDS485FrameCollector {
  public:
    BusInterfaceHandler(DSS* _pDSS, ModelMaintenance& _modelMaintenance);
    virtual void execute();
    virtual void initialize();

    void addFrameBucket(FrameBucketBase* _bucket);
    void removeFrameBucket(FrameBucketBase* _bucket);

    virtual void collectFrame(boost::shared_ptr<DS485CommandFrame> _frame);
  protected:
    virtual void doStart();
  private:
    void raiseModelEvent(ModelEvent* _pEvent);
    void dumpFrame(boost::shared_ptr<DS485CommandFrame> _pFrame);
  private:
    ModelMaintenance& m_ModelMaintenance;
    Mutex m_IncomingFramesGuard;
    Mutex m_FrameBucketsGuard;
    SyncEvent m_PacketHere;
    CommandFrameSharedPtrVector m_IncomingFrames;
    std::vector<FrameBucketBase*> m_FrameBuckets;
  };


} //  namespace dss
#endif /* BUSINTERFACEHANDLER_H_ */
