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

#ifndef PLUGINBASE_H_
#define PLUGINBASE_H_

#include "../include/dsid_plugin.h"

#include <string>
#include <map>
#include <stdexcept>

class DSID {
  private:
  public:
    virtual ~DSID() {}
    virtual void callScene(const int _sceneNr) = 0;
    virtual void saveScene(const int _sceneNr) = 0;
    virtual void undoScene(const int _sceneNr) = 0;

    virtual void increaseValue(const int _parameterNr = -1) = 0;
    virtual void decreaseValue(const int _parameterNr = -1) = 0;

    virtual void enable() = 0;
    virtual void disable() = 0;
    virtual int getFunctionID() { return 0; }

    virtual void startDim(bool _directionUp, const int _parameterNr = -1) = 0;
    virtual void endDim(const int _parameterNr = -1) = 0;
    virtual void setValue(const double _value, int _parameterNr = -1) = 0;

    virtual double getValue(int _parameterNr = -1) const = 0;

    virtual void setConfigurationParameter(const std::string& _name, const std::string& _value) = 0;
    virtual unsigned char udiSend(unsigned char _value, bool _lastByte) { return 0; }
};

class DSIDFactory {
private:
  std::map<int, DSID*> m_DSIDs;
  int m_NextHandle;
  std::string m_PluginName;
private:

protected:
  static DSIDFactory* m_Instance;
  virtual DSID* doCreateDSID() = 0;

  DSIDFactory(const std::string& _pluginName)
  : m_NextHandle(1),
    m_PluginName(_pluginName)
  { }

  virtual ~DSIDFactory() {};
public:
  static DSIDFactory& getInstance() {
    if(m_Instance == NULL) {
      throw std::runtime_error("No Factory created yet");
    }
    return *m_Instance;
  }

  int createDSID() {
    DSID* newDSID = doCreateDSID();
    m_DSIDs[m_NextHandle] = newDSID;
    return m_NextHandle++;
  }

  void destroyDSID(const int _handle) {
  }

  DSID* getDSID(const int _handle) {
    return m_DSIDs[_handle];
  }

  const std::string& getPluginName() const { return m_PluginName; }
}; // DSIDFactory


#endif /* PLUGINBASE_H_ */
