/*
    Copyright (c) 2009,2010,2012 digitalSTROM.org, Zurich, Switzerland
    Copyright (c) 2008 Patrick Staehlin <me@packi.ch>

    Authors: Patrick Staehlin, futureLAB AG <pstaehlin@futurelab.ch>
            Christian Hitz, aizo AG <christian.hitz@aizo.com>
            Sergey 'Jin' Bostandzhyan <jin@dev.digitalstrom.org>


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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "property-parser.h"

#include <boost/thread/recursive_mutex.hpp>

#include "src/base.h"

namespace dss {

const int PROPERTY_FORMAT_VERSION = 1;

//=============================================== PropertyParser
PropertyParser::PropertyParser() : ExpatParser(), m_level(0),
    m_ignoreVersion(false),
    m_expectValue(false),
    m_ignore(false),
    m_currentValueType(vTypeNone)
{};

// this callback is triggered on each <tag>
void PropertyParser::elementStart(const char *_name, const char **_attrs)
{
    if (m_forceStop) {
        return;
    }

    try {
        m_currentNode->checkWriteAccess();

        // if it is the root document we need to perform some checks
        // 0 level node must be called "properties" and must have a version
        // attribute
        if (m_level == 0) {
            m_expectValue = false;
            // first node must be named "properties"
            if (strcmp(_name, "properties") != 0) {
                Logger::getInstance()->log("PropertySystem::loadFromXML: root node "
                                           "must be named properties!", lsError);
                m_forceStop = true;
                return;
            }

            if (!m_ignoreVersion) {
                // now check version
                bool versionFound = false;
                for (int i = 0; _attrs[i]; i += 2)
                {
                    if (strcmp(_attrs[i], "version") == 0) {
                        std::string version = _attrs[i + 1];
                        if (strToIntDef(version, -1) != PROPERTY_FORMAT_VERSION) {
                            Logger::getInstance()->log(std::string("PropertySystem::"
                                                                   "loadFromXML: Version mismatch, expected ") +
                                                       intToString(PROPERTY_FORMAT_VERSION) + " got " + version,
                                                       lsError);
                            m_forceStop = true;
                            return;
                        } else {
                            versionFound = true;
                        }
                    }
                }
                if (!versionFound) {
                    Logger::getInstance()->log("PropertySystem::loadFromXML: missing "
                                               "version attribute", lsError);
                    m_forceStop = true;
                    return;
                }
            }

            m_level++;
            return;
        }

        // only "property" nodes are allowed on level 1, deeper we could have
        // property or value nodes
        if (m_level == 1) {
            m_expectValue = false;
            m_currentValueType = vTypeNone;
            if (strcmp(_name, "property") != 0) {
                m_ignore = true;
                m_level++;
                return;
            } else {
                // be tolerant and simply ignore all non propety nodes on level 1
                m_ignore = false;
            }
        }

        // we are parsing a path that is being ignored, so do nothing
        if ((m_level > 1) && m_ignore)
        {
            m_level++;
            return;
        }

        // if we got here, then we are on a valid level and have valid nodes
        if (strcmp(_name, "property") == 0) {
            // first parse all propety nodes attributes
            const char* propName = NULL;
            const char* propType = NULL;
            const char* propWriteable = NULL;
            const char* propReadable = NULL;
            const char* propArchive = NULL;

            m_currentValueType = vTypeNone;

            for (int i = 0; _attrs[i]; i += 2)
            {
                if (strcmp(_attrs[i], "name") == 0) {
                    propName = _attrs[i + 1];
                } else if (strcmp(_attrs[i], "type") == 0) {
                    propType = _attrs[i + 1];
                } else if (strcmp(_attrs[i], "writable") == 0) {
                    propWriteable = _attrs[i + 1];
                } else if (strcmp(_attrs[i], "readable") == 0) {
                    propReadable = _attrs[i + 1];
                } else if (strcmp(_attrs[i], "archive") == 0) {
                    propArchive = _attrs[i + 1];
                }
            }

            // property name is mandatory
            if (propName == NULL) {
                Logger::getInstance()->log("PropertySystem::loadFromXML: property "
                                           "tag is missing the name attribute",
                                           lsError);
                m_forceStop = true;
                return;
            }

            if (propType != NULL) {
                m_currentValueType = getValueTypeFromString(propType);
            } else {
                m_currentValueType = vTypeNone;
            }

            // ok this is a special "proprety system" feature,basically one can have
            // node names that describe full paths, i.e. /path/to/node
            // for the above case only the last element is supposed to have an
            // associated value, the rest is just a path to the node. since that
            // path is not part of the actual XML we will have to take care of
            // generating this structure ourselves
            //
            // in theory the createProperty() method of the ProprtyNode class should
            // be able to take care of this, but it simply did not work in the way
            // I would have expected, the resulting tree structure became messed up.
            // so we'll be taking care of it manually here

            PropertyNodePtr node;
            PropertyNodePtr temp = m_currentNode;
            PropertyNodePtr path;

            const char *slash = strchr(propName, '/');
            const char *start = propName;
            do {
                // try to get first part of the node path
                std::string part;
                if (slash != NULL) {
                    part = std::string(start, slash - start);
                    start = slash + 1;
                    if (start != NULL) {
                        slash = strchr(start, '/');
                    }
                } else {
                    part = start;
                    start = NULL;
                }
                path.reset();

                part = part.substr(0, part.find('['));

                // this is important, if a node with the same name already exists we
                // have to reuse it, otherwise we will generate an array wich is
                // not what we want. root level requires additional handling, this is
                // what the property system seems to expect
                boost::recursive_mutex::scoped_lock lock(PropertyNode::m_GlobalMutex);
                if ((m_level == 1) && (m_currentNode->getName() == part)) {
                    path = m_currentNode;
                } else {
                    path = temp->getPropertyByName(part);
                }
                if (path == NULL) {
                    path = PropertyNodePtr(new PropertyNode(part.c_str()));
                    temp->addChild(path);
                }
                temp = path;
            } while (start != NULL);
            node = path;

            if (propWriteable != NULL)
            {
                node->setFlag(PropertyNode::Writeable,
                              strcmp(propWriteable, "true") == 0);
            }
            if (propReadable != NULL)
            {
                node->setFlag(PropertyNode::Readable,
                              strcmp(propReadable, "true") == 0);
            }
            if (propArchive != NULL)
            {
                node->setFlag(PropertyNode::Archive, strcmp(propArchive, "true") == 0);
            }

            // we're not pushing the interim nodes that may have been created above
            // because the XML does not know anything about the weirdo-property
            // tree structure
            m_nodes.push_back(m_currentNode);
            m_currentNode = node;
        } else if (strcmp(_name, "value") == 0)  { // we encountered a value tag
            if (m_currentValueType == vTypeNone) {
                Logger::getInstance()->log("PropertySystem::loadFromXML: missing "
                                           "value type for property", lsError);
                m_forceStop = true;
                return;
            }
            // tell the character data callback that it should do something useful
            m_expectValue = true;
        }

        m_level++;
    } catch (std::runtime_error& ex) {
        m_forceStop = true;
        Logger::getInstance()->log(std::string("PropertySystem::loadFromXML: ") +
                                   "element start handler caught exception: " + ex.what() +
                                   " Will abort parsing!", lsError);
    } catch (...) {
        m_forceStop = true;
        Logger::getInstance()->log("PropertySystem::loadFromXML: "
                                   "element start handler caught exception! Will abort parsing!",
                                   lsError);
    }
}

// this callback is triggered for each </tag>
void PropertyParser::elementEnd(const char *_name) {
    if (m_forceStop) {
        return;
    }

    m_level--;
    // this should NEVER happen and is therefore a good check/test to
    // detect bugs/error conditions that remained unhandled; I know that
    // rest of the proptree classes uses asserts here and there but that's
    // not really nice, so I prefer to tell the calling function that the
    // XML could not be parsed, instead of just crashing ;)
    if (m_level < 0) {
        Logger::getInstance()->log("PropertySystem::loadFromXML: elementEnd "
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
        // handle position in the document, we only put property nodes into the
        // stack
        if (strcmp(_name, "property") == 0) {
            m_currentNode = m_nodes.back();
            m_nodes.pop_back();
            m_currentValueType = vTypeNone;
        } else if (strcmp(_name, "value") == 0) {
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
            case vTypeUnsignedInteger:
                m_currentNode->setUnsignedIntegerValue(strToUInt(m_temporaryValue));
                break;
            case vTypeBoolean:
                m_currentNode->setBooleanValue(m_temporaryValue == "true");
                break;
            case vTypeFloating:
            {
                double value = ::strtod(m_temporaryValue.c_str(), 0);
                m_currentNode->setFloatingValue(value);
            }
            break;
            default:
                Logger::getInstance()->log("PropertySystem::loadFromXML: character "
                                           "data callback - unknown value type!");
                m_forceStop = true;
            }
        }
        m_temporaryValue.clear();
    } catch (std::runtime_error& ex) {
        m_forceStop = true;
        Logger::getInstance()->log(std::string("PropertySystem::loadFromXML: ") +
                                   "element end handler caught exception: " + ex.what() +
                                   " Will abort parsing!", lsError);
    } catch (...) {
        m_forceStop = true;
        Logger::getInstance()->log("PropertySystem::loadFromXML: "
                                   "element end handler caught exception! Will abort parsing!",
                                   lsError);

    }
}

// this callback gets triggered when we receive some character data between
// tags, i.e.: <tag>character data</tag>
void PropertyParser::characterData(const XML_Char *_s, int _len) {
    if (m_forceStop) {
        return;
    }

    // the property system only supports character data enclosed in a <value>
    // tag, everything else is ignored
    if (!m_expectValue) {
        return;
    }

    if (m_currentValueType == vTypeNone) {
        Logger::getInstance()->log("PropertySystem::loadFromXML: character "
                                   "data callback - received value data but "
                                   "value type is missing, ignoring");
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

void PropertyParser::reinitMembers(PropertyNodePtr _node,
                                   bool _ignoreVersion) {
    // the propety parser class instance can be reused, so we will reset
    // the internals on each call of the loadFromXML function
    m_level = 0;
    m_ignoreVersion = _ignoreVersion;
    m_expectValue = false;
    m_ignore = false;
    m_currentValueType = vTypeNone;
    m_currentNode = _node;
    m_temporaryValue.clear();
    m_nodes.clear();

    m_nodes.push_back(_node);
    m_currentNode = _node;
}

bool PropertyParser::loadFromXML(const std::string& _fileName,
                                 PropertyNodePtr _node, bool _ignoreVersion) {
    if (_node == NULL) {
        return false;
    }

    reinitMembers(_node, _ignoreVersion);

    bool ret = parseFile(_fileName);
    m_nodes.clear();
    return ret;
} // loadFromXML


PropertyParserProxy::PropertyParserProxy() : PropertyParser() {}

void PropertyParserProxy::elementStartCb(const char *_name,
        const char **_attrs) {
    elementStart(_name, _attrs);
}

void PropertyParserProxy::elementEndCb(const char *_name) {
    elementEnd(_name);
}

void PropertyParserProxy::characterDataCb(const XML_Char *_s, int _len) {
    characterData(_s, _len);
}

void PropertyParserProxy::reset(PropertyNodePtr _node, bool _ignoreVersion) {
    reinitMembers(_node, _ignoreVersion);
}

}
