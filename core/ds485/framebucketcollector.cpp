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

#include <boost/thread/locks.hpp>
#include <boost/date_time/posix_time/posix_time_duration.hpp>

#include "core/logger.h"
#include "core/base.h"

namespace dss {

  //================================================== FrameBucketCollector

  FrameBucketCollector::FrameBucketCollector(FrameBucketHolder* _holder, int _functionID, int _sourceID)
  : FrameBucketBase(_holder, _functionID, _sourceID),
    m_SingleFrame(false)
  { } // ctor

  bool FrameBucketCollector::addFrame(boost::shared_ptr<DS485CommandFrame> _frame) {
    bool result = false;
    {
      boost::mutex::scoped_lock lock(m_FramesMutex);
      if(!m_SingleFrame || m_Frames.empty()) {
        m_Frames.push_back(_frame);
        result = true;
      }
    }
    if(result) {
      m_EmptyCondition.notify_one();
      Logger::getInstance()->log("FrameBucket::addFrame:  fid: " + intToString(getFunctionID()) + " sid: " + intToString(getSourceID()));
    }

    return result;
  } // addFrame

  boost::shared_ptr<DS485CommandFrame> FrameBucketCollector::popFrame() {
    boost::shared_ptr<DS485CommandFrame> result;

    boost::mutex::scoped_lock lock(m_FramesMutex);
    if(!m_Frames.empty()) {
      result = m_Frames.front();
      m_Frames.pop_front();
    }
    return result;
  } // popFrame

  void FrameBucketCollector::waitForFrames(int _timeoutMS) {
    m_SingleFrame = false;
    sleepMS(_timeoutMS);
  } // waitForFrames


  struct HasFrames {
    std::deque<boost::shared_ptr<DS485CommandFrame> > & m_Frames;
    HasFrames(std::deque<boost::shared_ptr<DS485CommandFrame> >& _frames)
    : m_Frames(_frames)
    {}

    bool operator()() const {
      return !m_Frames.empty();
    }
  };

  bool FrameBucketCollector::waitForFrame(int _timeoutMS) {
    m_SingleFrame = true;
    if(isEmpty()) {

      boost::mutex::scoped_lock lock(m_FramesMutex);
      if(!m_EmptyCondition.timed_wait(lock, boost::posix_time::milliseconds(_timeoutMS), HasFrames(m_Frames))) {
        Logger::getInstance()->log("FrameBucketCollector::waitForFrame: No frame received");
        return false;
      };

      Logger::getInstance()->log("FrameBucketCollector::waitForFrame: Got frame");
      return !m_Frames.empty();
    }
    return true;
  } // waitForFrame

  int FrameBucketCollector::getFrameCount() const {
    boost::mutex::scoped_lock lock(m_FramesMutex);
    return m_Frames.size();
  } // getFrameCount

  bool FrameBucketCollector::isEmpty() const {
    boost::mutex::scoped_lock lock(m_FramesMutex);
    return m_Frames.empty();
  } // isEmpty


} // namespace dss
