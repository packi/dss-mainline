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
    virtual void CallScene(const int _sceneNr) = 0;
    virtual void SaveScene(const int _sceneNr) = 0;
    virtual void UndoScene(const int _sceneNr) = 0;

    virtual void IncreaseValue(const int _parameterNr = -1) = 0;
    virtual void DecreaseValue(const int _parameterNr = -1) = 0;

    virtual void Enable() = 0;
    virtual void Disable() = 0;

    virtual void StartDim(bool _directionUp, const int _parameterNr = -1) = 0;
    virtual void EndDim(const int _parameterNr = -1) = 0;
    virtual void SetValue(const double _value, int _parameterNr = -1) = 0;

    virtual double GetValue(int _parameterNr = -1) const = 0;

    virtual void SetConfigurationParameter(const std::string& _name, const std::string& _value) = 0;
};

class DSIDFactory {
private:
  std::map<int, DSID*> m_DSIDs;
  int m_NextHandle;
  std::string m_PluginName;
private:

protected:
  static DSIDFactory* m_Instance;
  virtual DSID* DoCreateDSID() = 0;

  DSIDFactory(const std::string& _pluginName)
  : m_NextHandle(1),
    m_PluginName(_pluginName)
  { }

  virtual ~DSIDFactory() {};
public:
  static DSIDFactory& GetInstance() {
    if(m_Instance == NULL) {
      throw std::runtime_error("No Factory created yet");
    }
    return *m_Instance;
  }

  int CreateDSID() {
    DSID* newDSID = DoCreateDSID();
    m_DSIDs[m_NextHandle] = newDSID;
    return m_NextHandle++;
  }

  void DestroyDSID(const int _handle) {
  }

  DSID* GetDSID(const int _handle) {
    return m_DSIDs[_handle];
  }

  const std::string& GetPluginName() const { return m_PluginName; }
}; // DSIDFactory


#endif /* PLUGINBASE_H_ */
