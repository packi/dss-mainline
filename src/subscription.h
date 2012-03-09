/*
    Copyright (c) 2012 digitalSTROM.org, Zurich, Switzerland

    Authors: Michael Troß, aizo GmbH <michael.tross@aizo.com>

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

#ifndef SUBSCRIPTION_H_
#define SUBSCRIPTION_H_

#include <stdexcept>
#include <vector>
#include <stack>
#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/thread/recursive_mutex.hpp>

#include "expatparser.h"

namespace dss {

  class PropertyNode;
  typedef boost::shared_ptr<PropertyNode> PropertyNodePtr;

  class SubscriptionParser : public ExpatParser
  {
    public:
      SubscriptionParser(PropertyNodePtr _subs, PropertyNodePtr _states);
      bool loadFromXML(const std::string& _fileName,
                       bool _ignoreVersion = false);

      PropertyNodePtr getSubscriptionNode() { return m_subscriptionNode; }
      PropertyNodePtr getStatesNode() { return m_statesNode; }

    private:
      int m_level;
      bool m_ignoreVersion;
      bool m_expectValue;
      bool m_ignore;

      int m_nSubscriptions;
      int m_nFilter;

      std::stack<PropertyNodePtr> m_nodes;
      PropertyNodePtr m_currentNode;
      aValueType m_currentValueType;

      std::string m_optionString;
      std::string m_temporaryValue;

      PropertyNodePtr m_subscriptionNode;
      PropertyNodePtr m_statesNode;

      void clearStack();
    protected:
      void reinitMembers(bool ignoreVersion = false);
      virtual void elementStart(const char *_name, const char **_attrs);
      virtual void elementEnd(const char *_name);
      virtual void characterData(const XML_Char *_s, int _len);
  };

  class SubscriptionParserProxy : public SubscriptionParser
  {
  public:
    SubscriptionParserProxy(PropertyNodePtr _subs, PropertyNodePtr _states);
    void reset(bool ignoreVersion = false);
    void elementStartCb(const char *_name, const char **_attrs);
    void elementEndCb(const char *_name);
    void characterDataCb(const XML_Char *_s, int _len);
  };

};

#endif /* SUBSCRIPTION_H_ */
