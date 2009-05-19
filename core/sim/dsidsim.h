 /*
 * dsidsim.h
 *
 *  Created on: May 18, 2009
 *      Author: patrick
 */

#ifndef DSIDSIM_H_
#define DSIDSIM_H_

#include "dssim.h"

namespace dss {

  class DSIDSim : public DSIDInterface {
  private:
    bool m_On;
    bool m_Enabled;
    bool m_Dimming;
    time_t m_DimmStartTime;
    bool m_DimmingUp;
    vector<int> m_Parameters;
    DSModulatorSim* m_Modulator;
    vector<uint8_t> m_ValuesForScene;
    uint8_t m_CurrentValue;
    int m_DimTimeMS;
    int m_SimpleConsumption;
    Properties m_ConfigParameter;
  public:
    DSIDSim(const DSModulatorSim& _simulator, const dsid_t _dsid, const devid_t _shortAddress);
    virtual ~DSIDSim() {}

    virtual void CallScene(const int _sceneNr);
    virtual void SaveScene(const int _sceneNr);
    virtual void UndoScene(const int _sceneNr);

    virtual void IncreaseValue(const int _parameterNr = -1);
    virtual void DecreaseValue(const int _parameterNr = -1);

    virtual void Enable();
    virtual void Disable();

    virtual int GetConsumption();

    virtual void StartDim(bool _directionUp, const int _parameterNr = -1);
    virtual void EndDim(const int _parameterNr = -1);
    virtual void SetValue(const double _value, int _parameterNr = -1);

    virtual double GetValue(int _parameterNr = -1) const;

    virtual uint8_t GetFunctionID();
    void SetSimpleConsumption(const int _value) { m_SimpleConsumption = _value; }
    virtual void SetConfigParameter(const string& _name, const string& _value);
    virtual string GetConfigParameter(const string& _name) const;
  }; // DSIDSim


  class DSIDSimSwitch : public DSIDSim {
  private:
    const int m_NumberOfButtons;
    int m_DefaultColor;
    bool m_IsBell;
  public:
    DSIDSimSwitch(const DSModulatorSim& _simulator, const dsid_t _dsid, const devid_t _shortAddress, const int _numButtons)
    : DSIDSim(_simulator, _dsid, _shortAddress),
      m_NumberOfButtons(_numButtons),
      m_DefaultColor(GroupIDYellow),
      m_IsBell(false)
    {};
    ~DSIDSimSwitch() {}

    void PressKey(const ButtonPressKind _kind, const int _buttonNr);

    const int GetDefaultColor() const { return m_DefaultColor; }

    const int GetNumberOfButtons() const { return m_NumberOfButtons; }

    bool IsBell() const { return m_IsBell; }
    void SetIsBell(const bool _value) { m_IsBell = _value; }

    virtual uint8_t GetFunctionID() { return FunctionIDSwitch; }
  }; // DSIDSimSwitch
}

#endif /* DSIDSIM_H_ */
