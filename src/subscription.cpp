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

#include <iostream>
#include <sstream>
#include <algorithm>
#include <cassert>
#include <cstring>
#include <fstream>


#include "src/base.h"
#include "src/foreach.h"
#include "src/logger.h"

#include "dss.h"
#include "propertysystem.h"
#include "subscription.h"

#include "src/security/privilege.h"
#include "src/security/security.h"
#include "src/security/user.h"

namespace dss {

  const int SUBSCRIPTION_FORMAT_VERSION = 1;

  //=============================================== SubscriptionParser

  SubscriptionParser::SubscriptionParser(PropertyNodePtr _subs, PropertyNodePtr _states) :
      ExpatParser(),
      m_level(0),
      m_ignoreVersion(false),
      m_expectValue(false),
      m_ignore(false),
      m_nSubscriptions(0),
      m_currentValueType(vTypeNone) {
    m_subscriptionNode = _subs;
    m_statesNode = _states;
  };

  // this callback is triggered on each <tag>
  void SubscriptionParser::elementStart(const char *_name, const char **_attrs) {
    if (m_forceStop) {
      return;
    }

    try {

      // root level 0, first node must be named "subscriptions"
      if (m_level == 0) {
        m_expectValue = false;

        if (strcmp(_name, "subscriptions") != 0) {
          Logger::getInstance()->log("SubscriptionParser: root node "
                                     "must be subscriptions!", lsError);
          m_forceStop = true;
          return;
        }

        if (!m_ignoreVersion) {
          bool versionFound = false;
          for (int i = 0; _attrs[i]; i += 2)
          {
            if (strcmp(_attrs[i], "version") == 0) {
              std::string version = _attrs[i + 1];
              if (strToIntDef(version, -1) != SUBSCRIPTION_FORMAT_VERSION) {
                Logger::getInstance()->log(std::string("SubscriptionParser::"
                    "loadFromXML: Version mismatch, expected ") +
                    intToString(SUBSCRIPTION_FORMAT_VERSION) + " got " + version,
                    lsError);
                m_forceStop = true;
                return;
              } else {
                versionFound = true;
              }
            }
          }
          if (!versionFound) {
            Logger::getInstance()->log("SubscriptionParser::loadFromXML: missing "
                                       "version attribute", lsError);
            m_forceStop = true;
            return;
          }
        }

        m_level++;
        return;
      }

      // on level 1 subscription and state are allowed
      if (m_level == 1) {
        m_expectValue = false;

        if (strcmp(_name, "subscription") == 0) {
          m_ignore = false;
          m_level++;
        } else if (strcmp(_name, "state") == 0) {
          m_ignore = false;
          m_level++;
        } else {
          m_ignore = true;
          return;
        }
      }

      // we are parsing a path that is being ignored, so do nothing
      if ((m_level > 1) && m_ignore)
      {
        m_level++;
        return;
      }

      // top level 2, subscription or state
      if (m_level == 2) {
        m_nFilter = 0;
        m_optionString.clear();

        if (strcmp(_name, "subscription") == 0) {
          const char* eventName = NULL;
          const char* handlerName = NULL;
          for (int i = 0; _attrs[i]; i += 2)
          {
            if (strcmp(_attrs[i], "event-name") == 0) {
              eventName = _attrs[i + 1];
            } else if (strcmp(_attrs[i], "handler-name") == 0) {
              handlerName = _attrs[i + 1];
            }
          }
          if (eventName == NULL || handlerName == NULL) {
            Logger::getInstance()->log("SubscriptionParser::loadFromXML: subscription "
                "tag is missing the event-name or handler-name attribute",
                lsError);
            m_forceStop = true;
            return;
          }
          if (m_subscriptionNode == NULL) {
            Logger::getInstance()->log("loadState: no root subscription node, skipping", lsWarning);
            return;
          }
          m_nSubscriptions++;
          m_currentNode.reset(new PropertyNode(intToString(m_nSubscriptions).c_str()));
          m_currentNode->createProperty("event-name")->setStringValue(eventName);
          m_currentNode->createProperty("handler-name")->setStringValue(handlerName);
          m_subscriptionNode->addChild(m_currentNode);

        } else if (strcmp(_name, "state") == 0)  {
          const char* stateName = NULL;
          for (int i = 0; _attrs[i]; i += 2)
          {
            if (strcmp(_attrs[i], "name") == 0) {
              stateName = _attrs[i + 1];
            }
          }
          if (stateName == NULL) {
            Logger::getInstance()->log("SubscriptionParser::loadFromXML: "
                "state tag is missing the name", lsError);
            m_forceStop = true;
            return;
          }
          if (m_statesNode == NULL) {
            Logger::getInstance()->log("SubscriptionParser::loadFromXML: "
                "no root state node, skipping states", lsWarning);
            return;
          }
          if (m_statesNode->getPropertyByName(stateName)) {
            Logger::getInstance()->log("SubscriptionParser::loadFromXML: "
                "duplicate state name, skipping this state", lsWarning);
            m_forceStop = true;
            return;
          }
          m_currentNode.reset(new PropertyNode(stateName));
          m_currentNode->createProperty("name")->setStringValue(stateName);
          m_statesNode->addChild(m_currentNode);

        } else {
          Logger::getInstance()->log("SubscriptionParser::loadFromXML: "
              "expected either subscription or state tag",
              lsError);
          m_forceStop = true;
          return;
        }
        m_level++;

      // parameter/property level 3
      } else if (m_level >= 3) {

        if ((strcmp(_name, "parameter") == 0) || (strcmp(_name, "property") == 0)) {
          const char* pName = _name;
          const char* pType = NULL;

          for (int i = 0; _attrs[i]; i += 2)
          {
            if (strcmp(_attrs[i], "name") == 0) {
              pName = _attrs[i + 1];
            } else if (strcmp(_attrs[i], "type") == 0) {
              pType = _attrs[i + 1];
            }
          }

          if (pType != NULL) {
            m_currentValueType = getValueTypeFromString(pType);
          } else {
            m_currentValueType = vTypeString;
          }

          PropertyNodePtr temp = m_currentNode;
          PropertyNodePtr path = PropertyNodePtr(new PropertyNode(pName));
          temp->addChild(path);

          m_nodes.push(m_currentNode);
          m_currentNode = path;
          m_level++;

          // tell the character data callback that it should do something useful
          m_temporaryValue.clear();
          m_expectValue = true;

        } else if (strcmp(_name, "filter") == 0) {
          for (int i = 0; _attrs[i]; i += 2) {
            if (strcmp(_attrs[i], "match") == 0) {
              m_optionString = _attrs[i + 1];
            }
          }
          PropertyNodePtr filter = PropertyNodePtr(new PropertyNode("filter"));
          filter->setStringValue(m_optionString);
          m_currentNode->addChild(filter);

          m_nodes.push(m_currentNode);
          m_currentNode = filter;
          m_level++;

        } else if (strcmp(_name, "property-filter") == 0) {
          const char* fType;
          const char* fProperty;
          const char* fValue;
          fType = fValue = fProperty = NULL;

          for (int i = 0; _attrs[i]; i += 2) {
            if (strcmp(_attrs[i], "type") == 0) {
              fType = _attrs[i + 1];
            } else if (strcmp(_attrs[i], "value") == 0) {
              fValue = _attrs[i + 1];
            } else if (strcmp(_attrs[i], "property") == 0) {
              fProperty = _attrs[i + 1];
            }
          }

          if (fType == NULL || fProperty == NULL) {
            Logger::getInstance()->log("SubscriptionParser::loadFromXML: subscription "
                "property-filter is missing the type or property attribute",
                lsError);
            m_forceStop = true;
            return;
          }

          PropertyNodePtr pProp = PropertyNodePtr(new PropertyNode("property"));
          m_currentNode->addChild(pProp);
          PropertyNodePtr pTemp = PropertyNodePtr(new PropertyNode("match"));
          pTemp->setStringValue(m_optionString);
          pProp->addChild(pTemp);
          pTemp = PropertyNodePtr(new PropertyNode("type"));
          pTemp->setStringValue(fType);
          pProp->addChild(pTemp);
          pTemp = PropertyNodePtr(new PropertyNode("property"));
          pTemp->setStringValue(fProperty);
          pProp->addChild(pTemp);
          if (fValue) {
            pTemp = PropertyNodePtr(new PropertyNode("value"));
            pTemp->setStringValue(fValue);
            pProp->addChild(pTemp);
          }

          m_level++;

        } else {
          m_ignore = true;
          Logger::getInstance()->log("SubscriptionParser::loadFromXML: "
              "ignoring: " + std::string(_name));
        }
      }

    } catch (std::runtime_error& ex) {
      m_forceStop = true;
      Logger::getInstance()->log(std::string("SubscriptionParser::loadFromXML: ") +
              "element start handler caught exception: " + ex.what() +
              " Will abort parsing!", lsError);
    } catch (...) {
      m_forceStop = true;
      Logger::getInstance()->log("SubscriptionParser::loadFromXML: "
              "element start handler caught exception! Will abort parsing!",
              lsError);
    }
  }

  // this callback is triggered for each </tag>
  void SubscriptionParser::elementEnd(const char *_name) {
    if (m_forceStop) {
      return;
    }

    m_level--;
    if (m_level < 0) {
        Logger::getInstance()->log("SubscriptionParser::loadFromXML: elementEnd "
                                   "invalid document depth!", lsError);
        m_forceStop = true;
        return;
    }

    // we can't have several value tags one after the other, so if we
    // got into a <value> then we do not expect any further <value> tags
    m_expectValue = false;

    // if we got into an "unsupported" branch, i.e. level 1 was not a <property>
    // then we simply ignore it
    if (m_ignore) {
      return;
    }

    try {
      if (m_currentValueType != vTypeNone) {
        switch (m_currentValueType) {
        case vTypeString:
          // we may receive one string through two callbacks depending when
          // expat hits the buffer boundary
          m_currentNode->setStringValue(m_temporaryValue);
          // reset flag if we were able to set a string value
          break;
        case vTypeInteger:
          m_currentNode->setIntegerValue(strToInt(m_temporaryValue));
          break;
        case vTypeBoolean:
          m_currentNode->setBooleanValue(m_temporaryValue == "true");
          break;
        default:
          break;
        }
      }
      if ((strcmp(_name, "property") == 0) ||
          (strcmp(_name, "parameter") == 0) ||
          (strcmp(_name, "filter") == 0)) {
        m_currentNode = m_nodes.top();
        m_nodes.pop();
        m_currentValueType = vTypeNone;
      }
      m_temporaryValue.clear();
    } catch (std::runtime_error& ex) {
      m_forceStop = true;
      Logger::getInstance()->log(std::string("SubscriptionParser::loadFromXML: ") +
              "element end handler caught exception: " + ex.what() +
              " Will abort parsing!", lsError);
    } catch (...) {
      m_forceStop = true;
      Logger::getInstance()->log("SubscriptionParser::loadFromXML: "
              "element end handler caught exception! Will abort parsing!",
              lsError);

    }
  }

  // this callback gets triggered when we receive some character data between
  // tags, i.e.: <tag>character data</tag>
  void SubscriptionParser::characterData(const XML_Char *_s, int _len) {
    if (m_forceStop) {
      return;
    }

    // the property system only supports character data enclosed in a <value>
    // tag, everything else is ignored
    if (!m_expectValue) {
      return;
    }

    std::string value = std::string(_s, _len);
    // the empty check is probably not needed, if I understand expat correctly
    // we won't get called if there is no data available, still better be safe
    // than sorry
    if (!value.empty() && (m_currentValueType != vTypeNone))
    {
      m_temporaryValue += value;
    }
  }

  void SubscriptionParser::clearStack() {
    while (!m_nodes.empty()) {
      m_nodes.pop();
    }
  }

  void SubscriptionParser::reinitMembers(bool _ignoreVersion) {
    m_level = 0;
    m_ignoreVersion = _ignoreVersion;
    m_expectValue = false;
    m_ignore = false;
    m_currentValueType = vTypeNone;
    m_currentNode.reset();
    m_temporaryValue.clear();
    clearStack();
  }

  bool SubscriptionParser::loadFromXML(const std::string& _fileName,
                                       bool _ignoreVersion) {
    Logger::getInstance()->log(std::string("SubscriptionParser: loading from ") + _fileName);

    reinitMembers(_ignoreVersion);

    bool ret = parseFile(_fileName);
    clearStack();
    return ret;
  } // loadFromXML


  SubscriptionParserProxy::SubscriptionParserProxy(PropertyNodePtr _subs, PropertyNodePtr _states) :
      SubscriptionParser(_subs, _states) {
  }

  void SubscriptionParserProxy::elementStartCb(const char *_name,
                                               const char **_attrs) {
    elementStart(_name, _attrs);
  }

  void SubscriptionParserProxy::elementEndCb(const char *_name) {
    elementEnd(_name);
  }

  void SubscriptionParserProxy::characterDataCb(const XML_Char *_s, int _len) {
    characterData(_s, _len);
  }

  void SubscriptionParserProxy::reset(bool _ignoreVersion) {
    reinitMembers(_ignoreVersion);
  }

};
