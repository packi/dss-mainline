/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland

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

#ifndef DSIDSIM_H_
#define DSIDSIM_H_

#include "dssim.h"

#include "core/base.h"

namespace dss {

  class DSIDSim : public DSIDInterface {
  private:
    bool m_On;
    bool m_Enabled;
    bool m_Dimming;
    time_t m_DimmStartTime;
    bool m_DimmingUp;
    std::vector<int> m_Parameters;
    DSDSMeterSim* m_DSMeter;
    std::vector<uint8_t> m_ValuesForScene;
    uint8_t m_CurrentValue;
    int m_DimTimeMS;
    int m_SimpleConsumption;
    Properties m_ConfigParameter;
  public:
    DSIDSim(const DSDSMeterSim& _simulator, const dss_dsid_t _dsid, const devid_t _shortAddress);
    virtual ~DSIDSim() {}

    virtual void callScene(const int _sceneNr);
    virtual void saveScene(const int _sceneNr);
    virtual void undoScene(const int _sceneNr);

    virtual void increaseValue(const int _parameterNr = -1);
    virtual void decreaseValue(const int _parameterNr = -1);

    virtual void enable();
    virtual void disable();

    virtual int getConsumption();

    virtual void startDim(bool _directionUp, const int _parameterNr = -1);
    virtual void endDim(const int _parameterNr = -1);
    virtual void setValue(const double _value, int _parameterNr = -1);

    virtual double getValue(int _parameterNr = -1) const;

    virtual uint16_t getFunctionID();
    void setSimpleConsumption(const int _value) { m_SimpleConsumption = _value; }
    virtual void setConfigParameter(const std::string& _name, const std::string& _value);
    virtual std::string getConfigParameter(const std::string& _name) const;
  }; // DSIDSim


  class DSIDSimSwitch : public DSIDSim {
  private:
    const int m_NumberOfButtons;
    int m_DefaultColor;
    bool m_IsBell;
  public:
    DSIDSimSwitch(const DSDSMeterSim& _simulator, const dss_dsid_t _dsid, const devid_t _shortAddress, const int _numButtons)
    : DSIDSim(_simulator, _dsid, _shortAddress),
      m_NumberOfButtons(_numButtons),
      m_DefaultColor(GroupIDYellow),
      m_IsBell(false)
    {};
    ~DSIDSimSwitch() {}

    void pressKey(const ButtonPressKind _kind, const int _buttonNr);

    const int getDefaultColor() const { return m_DefaultColor; }

    const int getNumberOfButtons() const { return m_NumberOfButtons; }

    bool isBell() const { return m_IsBell; }
    void setIsBell(const bool _value) { m_IsBell = _value; }

    virtual uint16_t getFunctionID() { return FunctionIDSwitch; }
  }; // DSIDSimSwitch
}

#endif /* DSIDSIM_H_ */
