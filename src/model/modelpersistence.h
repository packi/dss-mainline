/*
    Copyright (c) 2010,2012 digitalSTROM.org, Zurich, Switzerland

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


#ifndef MODELPERSISTENCE_H_
#define MODELPERSISTENCE_H_

#include <iosfwd>
#include <boost/shared_ptr.hpp>
#include <sstream>

#include "expatparser.h"

namespace dss {

  class Apartment;
  class Zone;
  class Group;
  class Device;
  class DSMeter;
  class PropertyParserProxy;

  class ModelPersistence : public ExpatParser
  {
  public:
    ModelPersistence(Apartment& _apartment);
    virtual ~ModelPersistence();

    void readConfigurationFromXML(const std::string& _fileName);
    void writeConfigurationToXML(const std::string& _fileName);
  private:
    Apartment& m_Apartment;
  protected:
    enum parseState
    {
        ps_none,
        ps_apartment,
        ps_device,
        ps_meter,
        ps_zone,
        ps_group,
        ps_scene,
        ps_properties,
        ps_sensor
    };
    bool m_ignore;
    bool m_expectString;
    int m_level;
    int m_state;
    int m_tempScene;
    std::string m_chardata;
    boost::shared_ptr<Device> m_tempDevice;
    boost::shared_ptr<DSMeter> m_tempMeter;
    boost::shared_ptr<Zone> m_tempZone;
    boost::shared_ptr<Group> m_tempGroup;
    boost::shared_ptr<PropertyParserProxy> m_propParser;

    void parseDevice(const char *_name, const char **_attrs);
    void parseZone(const char *_name, const char **_attrs);
    void parseGroup(const char *_name, const char **_attrs);
    void parseMeter(const char *_name, const char **_attrs);
    void parseScene(const char *_name, const char **_attrs);
    void parseSensor(const char *_name, const char **_attrs);
    const char *getSingleAttribute(const char *_name, const char **_attrs);
    virtual void elementStart(const char *_name, const char **_attrs);
    virtual void elementEnd(const char *_name);
    virtual void characterData(const XML_Char *_s, int _len);
  }; // ModelPersistence

} // namespace dss

#endif /* MODELPERSISTENCE_H_ */
