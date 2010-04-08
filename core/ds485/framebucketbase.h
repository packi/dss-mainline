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

#ifndef FRAMEBUCKETBASE_H_
#define FRAMEBUCKETBASE_H_

#include <boost/shared_ptr.hpp>

namespace dss {

  class FrameBucketHolder;
  class DS485CommandFrame;

  /** A frame bucket gets notified on every frame that matches any given
   *  function-/source-id pair.
   *  If \a m_SourceID is -1 every source matches. */
  class FrameBucketBase {
  public:
    FrameBucketBase(FrameBucketHolder* _holder, int _functionID, int _sourceID);
    virtual ~FrameBucketBase() {}

    int getFunctionID() const { return m_FunctionID; }
    int getSourceID() const { return m_SourceID; }

    virtual bool addFrame(boost::shared_ptr<DS485CommandFrame> _frame) = 0;

    /** Registers the bucket at m_pHolder */
    void addToHolder();
    /** Removes the bucket from m_pHolder */
    void removeFromHolder();
    /** Static function to be used from a boost::shared_ptr as a deleter. */
    static void removeFromHolderAndDelete(FrameBucketBase* _obj);
  private:
    FrameBucketHolder* m_pHolder;
    int m_FunctionID;
    int m_SourceID;
  }; // FrameBucketBase

} // namespace dss

#endif /* FRAMEBUCKETBASE_H_ */
