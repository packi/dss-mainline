/*
    Copyright (c) 2009,2010,2012 digitalSTROM.org, Zurich, Switzerland
    Copyright (c) 2008 Patrick Staehlin <me@packi.ch>

    Authors: Patrick Staehlin, futureLAB AG <pstaehlin@futurelab.ch>
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
#pragma once

#include "propertysystem.h"
#include "expatparser.h"

namespace dss {

class PropertyParser : public ExpatParser
{
public:
    PropertyParser();
    bool loadFromXML(const std::string& _fileName, PropertyNodePtr _node,
                     bool _ignoreVersion = false);

private:
    int m_level;
    bool m_ignoreVersion;
    bool m_expectValue;
    bool m_ignore;
    aValueType m_currentValueType;
    std::vector<PropertyNodePtr> m_nodes;
    PropertyNodePtr m_currentNode;
    std::string m_temporaryValue;

protected:
    void reinitMembers(PropertyNodePtr _node, bool ignoreVersion = false);
    virtual void elementStart(const char *_name, const char **_attrs);
    virtual void elementEnd(const char *_name);
    virtual void characterData(const XML_Char *_s, int _len);
};

class PropertyParserProxy : public PropertyParser
{
public:
    PropertyParserProxy();
    void reset(PropertyNodePtr _node, bool ignoreVersion = false);
    void elementStartCb(const char *_name, const char **_attrs);
    void elementEndCb(const char *_name);
    void characterDataCb(const XML_Char *_s, int _len);
};

} // namespace dss
