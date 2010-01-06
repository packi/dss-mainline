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


#ifndef FRAMEBUCKETCOLLECTOR_H_
#define FRAMEBUCKETCOLLECTOR_H_

#include <deque>

#include <boost/shared_ptr.hpp>

#include "framebucketbase.h"
#include "core/mutex.h"
#include "core/syncevent.h"

namespace dss {

  /** FrameBucketCollector holds its received frames in a queue.
    */
  class FrameBucketCollector : public FrameBucketBase {
  private:
    std::deque<boost::shared_ptr<DS485CommandFrame> > m_Frames;
    SyncEvent m_PacketHere;
    Mutex m_FramesMutex;
    bool m_SingleFrame;
  public:
    FrameBucketCollector(BusInterfaceHandler* _proxy, int _functionID, int _sourceID);
    virtual ~FrameBucketCollector() { }

    /** Adds a DS485CommandFrame to the frames queue */
    virtual bool addFrame(boost::shared_ptr<DS485CommandFrame> _frame);
    /** Returns the least recently received item int the queue.
     * The pointer will contain NULL if isEmpty() returns true. */
    boost::shared_ptr<DS485CommandFrame> popFrame();

    /** Waits for frames to arrive for \a _timeoutMS */
    void waitForFrames(int _timeoutMS);
    /** Waits for a frame to arrive in \a _timeoutMS.
     * If a frame arrives earlier, the function returns */
    bool waitForFrame(int _timeoutMS);

    int getFrameCount() const;
    bool isEmpty() const;
  }; // FrameBucketCollector

} // namespace dss

#endif /* FRAMEBUCKETCOLLECTOR_H_ */
