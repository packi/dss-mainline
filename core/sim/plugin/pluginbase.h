/*
 * pluginbase.h
 *
 *  Created on: Apr 27, 2009
 *      Author: patrick
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

    virtual void startDim(bool _directionUp, const int _parameterNr = -1) = 0;
    virtual void endDim(const int _parameterNr = -1) = 0;
    virtual void setValue(const double _value, int _parameterNr = -1) = 0;

    virtual double getValue(int _parameterNr = -1) const = 0;

    virtual void setConfigurationParameter(const std::string& _name, const std::string& _value) = 0;
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
