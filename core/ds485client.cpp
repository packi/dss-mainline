/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland
    Copyright (c) 2009 futureLAB AG, Winterthur, Switzerland

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

#include "ds485client.h"

#include "core/dss.h"
#include "core/foreach.h"
#include "unix/ds485proxy.h"

namespace dss {

  //================================================== DS485ClientImpl

  class FrameBucketCallback;

  class DS485ClientImpl {
  public:
    std::vector<boost::shared_ptr<FrameBucketBase> > buckets;
  }; // DS485ClientImpl


  //================================================== FrameBucketCallback

  class FrameBucketCallback : public FrameBucketBase {
  public:
    FrameBucketCallback(DS485Proxy* _proxy, int _functionID, int _sourceID, DS485Client::FrameCallback_t _callback)
    : FrameBucketBase(_proxy, _functionID, _sourceID),
      m_callBack(_callback)
    {
      assert(_callback != NULL);
    } // ctor

    virtual bool addFrame(boost::shared_ptr<ReceivedFrame> _frame) {
      m_callBack(_frame->getFrame());
      return false;
    } // addFrame
  private:
    DS485Client::FrameCallback_t m_callBack;
  }; // FrameBucketCallback


  //================================================== DS485Client

  DS485Client::DS485Client()
  : m_pImpl(new DS485ClientImpl)
  { } // ctor

  void DS485Client::sendFrameDiscardResult(DS485CommandFrame& _frame) {
    DSS::getInstance()->getDS485Interface().sendFrame(_frame);
  } // sendFrameDiscardResult

  boost::shared_ptr<DS485CommandFrame> DS485Client::sendFrameSingleResult(DS485CommandFrame& _frame, int _functionID, int _timeoutMS) {
    DS485Proxy* proxy = dynamic_cast<DS485Proxy*>(&DSS::getInstance()->getDS485Interface());
    assert(proxy != NULL);

    boost::shared_ptr<DS485CommandFrame> result;
    boost::shared_ptr<FrameBucketCollector> bucket = proxy->sendFrameAndInstallBucket(_frame, _functionID);
    if(bucket->waitForFrame(_timeoutMS)) {
      result = bucket->popFrame()->getFrame();
    }
    return result;
  } // sendFrameSingleResult

  std::vector<boost::shared_ptr<DS485CommandFrame> > DS485Client::sendFrameMultipleResults(DS485CommandFrame& _frame, int _functionID, int _timeoutMS) {
    DS485Proxy* proxy = dynamic_cast<DS485Proxy*>(&DSS::getInstance()->getDS485Interface());
    assert(proxy != NULL);

    boost::shared_ptr<FrameBucketCollector> bucket = proxy->sendFrameAndInstallBucket(_frame, _functionID);
    bucket->waitForFrame(_timeoutMS);

    std::vector<boost::shared_ptr<DS485CommandFrame> > result;
    boost::shared_ptr<ReceivedFrame> frame;
    while((frame = bucket->popFrame()) != NULL) {
      result.push_back(frame->getFrame());
    }
    return result;
  } // sendFrameMultipleResults

  void DS485Client::subscribeTo(int _functionID, int _source, FrameCallback_t _callback) {
    assert(_callback != NULL);

    DS485Proxy* proxy = dynamic_cast<DS485Proxy*>(&DSS::getInstance()->getDS485Interface());
    assert(proxy != NULL);

    boost::shared_ptr<FrameBucketBase> bucket(new FrameBucketCallback(proxy, _functionID, _source, _callback));
    m_pImpl->buckets.push_back(bucket);
  } // subscribeTo

  void DS485Client::unsubscribeFrom(int _functionID, int _source) {
    for(std::vector<boost::shared_ptr<FrameBucketBase> >::iterator ipBucket = m_pImpl->buckets.begin(), e = m_pImpl->buckets.end();
        ipBucket != e; ++ipBucket) {
      if(((*ipBucket)->getFunctionID() == _functionID) &&
          ((*ipBucket)->getSourceID() == _source)) {
        m_pImpl->buckets.erase(ipBucket);
        break;
      }
    }
   } // unsubscribeFrom


} // namespace dss
