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

#ifndef DS485CLIENT_H_
#define DS485CLIENT_H_

#include <vector>

#include <boost/shared_ptr.hpp>

namespace dss {

  class DS485CommandFrame;

  class __attribute__ ((visibility("default"))) DS485Client {
  public:
    typedef void (*FrameCallback_t)(boost::shared_ptr<DS485CommandFrame>);

    /** Sends a frame and discards incoming frames */
    void SendFrameDiscardResult(DS485CommandFrame& _frame);
    /** Sends a frame and receives one response frame with the same functionID as \a _functionID within _timeoutMS */
    boost::shared_ptr<DS485CommandFrame> SendFrameSingleResult(DS485CommandFrame& _frame, int _functionID, int _timeoutMS);
    /** Sends a frame and receives all frames with the same functionID as \a _functionID within _timeoutMS */
    std::vector<boost::shared_ptr<DS485CommandFrame> > SendFrameMultipleResults(DS485CommandFrame& _frame, int _functionID, int _timeoutMS);
    /** Subscribes to all frames with the functionID \a _functionID and source \a _source. If \a _source is -1 all Frames will captured. \a _callback is called once per received frame. */
    void SubscribeTo(int _functionID, int _source, FrameCallback_t _callback);
    /** Unsubscribes the first description that matches \a _functionID and \a _source. */
    void UnsubscribeFrom(int _functionID, int _source);
  }; // DS485Client
}

#endif /* DS485CLIENT_H_ */
