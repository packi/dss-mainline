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

#include "framebucketcollector.h"

#include "core/logger.h"
#include "core/base.h"

namespace dss {

  //================================================== FrameBucketCollector

  FrameBucketCollector::FrameBucketCollector(DS485Proxy* _proxy, int _functionID, int _sourceID)
  : FrameBucketBase(_proxy, _functionID, _sourceID),
    m_SingleFrame(false)
  { } // ctor

  bool FrameBucketCollector::addFrame(boost::shared_ptr<DS485CommandFrame> _frame) {
    bool result = false;
    m_FramesMutex.lock();
    if(!m_SingleFrame || m_Frames.empty()) {
      m_Frames.push_back(_frame);
      result = true;
    }
    m_FramesMutex.unlock();

    if(result) {
      m_PacketHere.signal();
    }
    return result;
  } // addFrame

  boost::shared_ptr<DS485CommandFrame> FrameBucketCollector::popFrame() {
    boost::shared_ptr<DS485CommandFrame> result;

    m_FramesMutex.lock();
    if(!m_Frames.empty()) {
      result = m_Frames.front();
      m_Frames.pop_front();
    }
    m_FramesMutex.unlock();
    return result;
  } // popFrame

  void FrameBucketCollector::waitForFrames(int _timeoutMS) {
    sleepMS(_timeoutMS);
  } // waitForFrames

  bool FrameBucketCollector::waitForFrame(int _timeoutMS) {
    m_SingleFrame = true;
    if(m_Frames.empty()) {
      Logger::getInstance()->log("FrameBucket::waitForFrame: Waiting for frame");
      if(m_PacketHere.waitFor(_timeoutMS)) {
        Logger::getInstance()->log("FrameBucket::waitForFrame: Got frame");
      } else {
        Logger::getInstance()->log("FrameBucket::waitForFrame: No frame received");
        return false;
      }
    }
    return true;
  } // waitForFrame

  int FrameBucketCollector::getFrameCount() const {
    return m_Frames.size();
  } // getFrameCount

  bool FrameBucketCollector::isEmpty() const {
    return m_Frames.empty();
  } // isEmpty


} // namespace dss
