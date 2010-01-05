/*
    Copyright (c) 2010 digitalSTROM.org, Zurich, Switzerland

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


#ifndef MODELPERSISTENCE_H_
#define MODELPERSISTENCE_H_

#include <iosfwd>

namespace Poco {
  namespace XML {
    class Node;
  }
}

namespace dss {

  class Apartment;

  class ModelPersistence {
  public:
    ModelPersistence(Apartment& _apartment);
    virtual ~ModelPersistence();

    void readConfigurationFromXML(const std::string& _fileName);
    void writeConfigurationToXML(const std::string& _fileName);
  private:
    void loadDevices(Poco::XML::Node* _node);
    void loadModulators(Poco::XML::Node* _node);
    void loadZones(Poco::XML::Node* _node);
  private:
    Apartment& m_Apartment;
  }; // ModelPersistence

} // namespace dss

#endif /* MODELPERSISTENCE_H_ */
