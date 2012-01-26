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

#include "config.h"
#include "dsid_js.h"
#include "src/scripting/jshandler.h"
#include "src/scripting/jsmodel.h"
#include "src/scripting/jssocket.h"
#if HAVE_CURL
  #include "src/scripting/jscurl.h"
#endif
#include "src/scripting/jsproperty.h"
#include "src/scripting/scriptobject.h"
#include "src/dss.h"
#include "src/thread.h"
#include "src/foreach.h"

namespace dss {

  class DSIDJS : public DSIDInterface {
  public:
    DSIDJS(const DSMeterSim& _simulator, dss_dsid_t _dsid,
           devid_t _shortAddress, boost::shared_ptr<ScriptContext> _pContext,
           const std::vector<std::string>& _fileNames)
    : DSIDInterface(_simulator, _dsid, _shortAddress),
      m_pContext(_pContext),
      m_FileNames(_fileNames)
    {
      createJSDevice();
    }

    virtual ~DSIDJS() {}

    virtual void initialize() {
      if(m_pSelf != NULL) {
        ScriptLock lock(m_pContext);
        JSContextThread req(m_pContext);
        ScriptFunctionParameterList param(*m_pContext);
        param.add(getDSID().toString());
        param.add(getShortAddress());
        param.add(getZoneID());
        try {
          m_pSelf->callFunctionByName<void>("initialize", param);
        } catch(ScriptException& e) {
          Logger::getInstance()->log(std::string("DSIDJS: Error calling 'initialize' ") + e.what(), lsError);
        }
      }
    }

    virtual void callScene(const int _sceneNr) {
      if(m_pSelf != NULL) {
        try {
          ScriptLock lock(m_pContext);
          JSContextThread req(m_pContext);
          ScriptFunctionParameterList param(*m_pContext);
          param.add(_sceneNr);
          m_pSelf->callFunctionByName<void>("callScene", param);
        } catch(ScriptException& e) {
          Logger::getInstance()->log(std::string("DSIDJS: Error calling 'callScene'") + e.what(), lsError);
        }
      }
    } // callScene

    virtual void saveScene(const int _sceneNr) {
      if(m_pSelf != NULL) {
        try {
          ScriptLock lock(m_pContext);
          JSContextThread req(m_pContext);
          ScriptFunctionParameterList param(*m_pContext);
          param.add(_sceneNr);
          m_pSelf->callFunctionByName<void>("saveScene", param);
        } catch(ScriptException& e) {
          Logger::getInstance()->log(std::string("DSIDJS: Error calling 'saveScene'") + e.what(), lsError);
        }
      }
    } // saveScene

    virtual void undoScene(const int _sceneNr) {
      if(m_pSelf != NULL) {
        try {
          ScriptLock lock(m_pContext);
          JSContextThread req(m_pContext);
          ScriptFunctionParameterList param(*m_pContext);
          param.add(_sceneNr);
          m_pSelf->callFunctionByName<void>("undoScene", param);
        } catch(ScriptException& e) {
          Logger::getInstance()->log(std::string("DSIDJS: Error calling 'undoScene'") + e.what(), lsError);
        }
      }
    } // saveScene

    virtual void undoSceneLast() {
      if(m_pSelf != NULL) {
        try {
          ScriptLock lock(m_pContext);
          JSContextThread req(m_pContext);
          ScriptFunctionParameterList param(*m_pContext);
          m_pSelf->callFunctionByName<void>("undoSceneLast", param);
        } catch(ScriptException& e) {
          Logger::getInstance()->log(std::string("DSIDJS: Error calling 'undoSceneLast'") + e.what(), lsError);
        }
      }
    } // undoScene

    virtual void enable() {
      if(m_pSelf != NULL) {
        try {
          ScriptLock lock(m_pContext);
          JSContextThread req(m_pContext);
          ScriptFunctionParameterList param(*m_pContext);
          m_pSelf->callFunctionByName<void>("enable", param);
        } catch(ScriptException& e) {
          Logger::getInstance()->log(std::string("DSIDJS: Error calling 'enable'") + e.what(), lsError);
        }
      }
    } // enable

    virtual void disable() {
      if(m_pSelf != NULL) {
        try {
          ScriptLock lock(m_pContext);
          JSContextThread req(m_pContext);
          ScriptFunctionParameterList param(*m_pContext);
          m_pSelf->callFunctionByName<void>("disable", param);
        } catch(ScriptException& e) {
          Logger::getInstance()->log(std::string("DSIDJS: Error calling 'disable'") + e.what(), lsError);
        }
      }
    } // disable

    virtual uint32_t getPowerConsumption() {
      if(m_pSelf != NULL) {
        try {
          ScriptLock lock(m_pContext);
          JSContextThread req(m_pContext);
          ScriptFunctionParameterList param(*m_pContext);
          return m_pSelf->callFunctionByName<uint32_t>("getConsumption", param);
        } catch(ScriptException& e) {
          Logger::getInstance()->log(std::string("DSIDJS: Error calling 'getConsumption'") + e.what(), lsError);
        }
      }
      return 0;
    } // getPowerConsumption

    virtual uint32_t getEnergyMeterValue() {
      if(m_pSelf != NULL) {
        try {
          ScriptLock lock(m_pContext);
          JSContextThread req(m_pContext);
          ScriptFunctionParameterList param(*m_pContext);
          return m_pSelf->callFunctionByName<uint32_t>("getEnergyMeter", param);
        } catch(ScriptException& e) {
          Logger::getInstance()->log(std::string("DSIDJS: Error calling 'getEnergyMeter'") + e.what(), lsError);
        }
      }
      return 0;
    } // getEnergyMeter

    virtual void setValue(uint8_t _value) {
      if(m_pSelf != NULL) {
        try {
          ScriptLock lock(m_pContext);
          JSContextThread req(m_pContext);
          ScriptFunctionParameterList param(*m_pContext);
          param.add(int(_value));
          m_pSelf->callFunctionByName<void>("setValue", param);
        } catch(ScriptException& e) {
          Logger::getInstance()->log(std::string("DSIDJS: Error calling 'setValue'") + e.what(), lsError);
        }
      }
    } // setValue

    virtual void setDeviceConfig(uint8_t _configClass, uint8_t _configIndex,
                                 uint8_t _value) {
      if(m_pSelf != NULL) {
        try {
          ScriptLock lock(m_pContext);
          JSContextThread req(m_pContext);
          ScriptFunctionParameterList param(*m_pContext);
          param.add(int(_configClass));
          param.add(int(_configIndex));
          param.add(int(_value));
          m_pSelf->callFunctionByName<void>("setDeviceConfig", param);
        } catch(ScriptException& e) {
          Logger::getInstance()->log(std::string("DSIDJS: Error calling 'setDeviceConfig'") + e.what(), lsError);
        }
      }
    } // setDeviceConfig


    virtual uint8_t getDeviceConfig(uint8_t _configClass,
                              uint8_t _configIndex) {
      if(m_pSelf != NULL) {
        try {
          ScriptLock lock(m_pContext);
          JSContextThread req(m_pContext);
          ScriptFunctionParameterList param(*m_pContext);
          param.add(_configClass);
          param.add(_configIndex);
          return m_pSelf->callFunctionByName<uint8_t>("getDeviceConfig", param);
        } catch(ScriptException& e) {
          Logger::getInstance()->log(std::string("DSIDJS: Error calling 'getDeviceConfig'") + e.what(), lsError);
        }
      }
      return 0;
    } // getDeviceConfig

    virtual uint16_t getDeviceConfigWord(uint8_t _configClass,
                                       uint8_t _configIndex) {
      if(m_pSelf != NULL) {
        try {
          ScriptLock lock(m_pContext);
          JSContextThread req(m_pContext);
          ScriptFunctionParameterList param(*m_pContext);
          param.add(_configClass);
          param.add(_configIndex);
          return m_pSelf->callFunctionByName<uint16_t>("getDeviceConfigWord",
                                                      param);
        } catch(ScriptException& e) {
          Logger::getInstance()->log(std::string("DSIDJS: Error calling 'getDeviceConfigWord'") + e.what(), lsError);
        }
      }
      return 0;
    } // getDeviceConfigWord

    virtual void setDeviceProgMode(uint8_t _modeId) {
      if(m_pSelf != NULL) {
        try {
          ScriptLock lock(m_pContext);
          JSContextThread req(m_pContext);
          ScriptFunctionParameterList param(*m_pContext);
          param.add(int(_modeId));
          m_pSelf->callFunctionByName<void>("setDeviceProgMode", param);
        } catch(ScriptException& e) {
          Logger::getInstance()->log(std::string("DSIDJS: Error calling 'setDeviceProgMode'") + e.what(), lsError);
        }
      }
    } // setDeviceProgMode

    virtual uint32_t getDeviceSensorValue(uint8_t _sensorIndex) {
      if(m_pSelf != NULL) {
        try {
          ScriptLock lock(m_pContext);
          JSContextThread req(m_pContext);
          ScriptFunctionParameterList param(*m_pContext);
          param.add(_sensorIndex);
          return m_pSelf->callFunctionByName<uint32_t>("getSensorValue", param);
        } catch(ScriptException& e) {
          Logger::getInstance()->log(std::string("DSIDJS: Error calling 'getSensorValue'") + e.what(), lsError);
        }
      }
      return 0;
    } // getDeviceSensorValue

    virtual uint8_t getDeviceSensorType(uint8_t _sensorIndex) {
      if(m_pSelf != NULL) {
        try {
          ScriptLock lock(m_pContext);
          JSContextThread req(m_pContext);
          ScriptFunctionParameterList param(*m_pContext);
          param.add(_sensorIndex);
          return m_pSelf->callFunctionByName<uint8_t>("getSensorType", param);
        } catch(ScriptException& e) {
          Logger::getInstance()->log(std::string("DSIDJS: Error calling 'getSensorType'") + e.what(), lsError);
        }
      }
      return 0;
    } // getDeviceSensorType

    virtual void sensorPush(uint8_t _sensorType, uint16_t _sensorValue ) {
      if(m_pSelf != NULL) {
        try {
          ScriptLock lock(m_pContext);
          JSContextThread req(m_pContext);
          ScriptFunctionParameterList param(*m_pContext);
          param.add(_sensorType);
          param.add(_sensorValue);
          m_pSelf->callFunctionByName<void>("sensorPush", param);
        } catch(ScriptException& e) {
          Logger::getInstance()->log(std::string("DSIDJS: Error calling 'sensorPush'") + e.what(), lsError);
        }
      }
    } // sensorPush

    virtual std::pair<uint8_t, uint16_t> getTransmissionQuality() {
        return std::make_pair(rand() % 255, rand() % 255);
    } // getTransmissionQuality

    virtual uint16_t getFunctionID() {
      if(m_pSelf != NULL) {
        try {
          ScriptLock lock(m_pContext);
          JSContextThread req(m_pContext);
          ScriptFunctionParameterList param(*m_pContext);
          return m_pSelf->callFunctionByName<int>("getFunctionID", param);
        } catch(ScriptException& e) {
          Logger::getInstance()->log(std::string("DSIDJS: Error calling 'getFunctionID'") + e.what(), lsError);
        }
      }
      return 0;
    } // getFunctionID

    virtual uint16_t getProductID() {
      if(m_pSelf != NULL) {
        try {
          ScriptLock lock(m_pContext);
          JSContextThread req(m_pContext);
          ScriptFunctionParameterList param(*m_pContext);
          return m_pSelf->callFunctionByName<int>("getProductID", param);
        } catch(ScriptException& e) {
          Logger::getInstance()->log(std::string("DSIDJS: Error calling 'getProductID'") + e.what(), lsError);
        }
      }
      return 0;
    } // getProductID

    virtual uint16_t getProductRevision() {
      if(m_pSelf != NULL) {
        try {
          ScriptLock lock(m_pContext);
          JSContextThread req(m_pContext);
          ScriptFunctionParameterList param(*m_pContext);
          return m_pSelf->callFunctionByName<int>("getProductRevision", param);
        } catch(ScriptException& e) {
          Logger::getInstance()->log(std::string("DSIDJS: Error calling 'getProductRevision'") + e.what(), lsError);
        }
      }
      return 0;
    } // getProductID

    virtual void setConfigParameter(const std::string& _name, const std::string& _value) {
      if(m_pSelf != NULL) {
        try {
          ScriptLock lock(m_pContext);
          JSContextThread req(m_pContext);
          ScriptFunctionParameterList param(*m_pContext);
          param.add(_name);
          param.add(_value);
          m_pSelf->callFunctionByName<void>("setConfigParameter", param);
        } catch(ScriptException& e) {
          Logger::getInstance()->log(std::string("DSIDJS: Error calling 'setConfigParameter'") + e.what(), lsError);
        }
      }
    } // setConfigParameter

    virtual std::string getConfigParameter(const std::string& _name) const {
      if(m_pSelf != NULL) {
        try {
          ScriptLock lock(m_pContext);
          JSContextThread req(m_pContext);
          ScriptFunctionParameterList param(*m_pContext);
          param.add(_name);
          return m_pSelf->callFunctionByName<std::string>("getConfigParameter", param);
        } catch(ScriptException& e) {
          Logger::getInstance()->log(std::string("DSIDJS: Error calling 'getConfigParameter'") + e.what(), lsError);
        }
      }
      return "";
    } // getConfigParameter

  private:

    void createJSDevice() {
      try {
        jsval res = JSVAL_NULL;
        foreach(std::string script, m_FileNames) {
          Logger::getInstance()->log("DSIDJS::initialize: Evaluating :'" + script + "'");
          res = m_pContext->doEvaluateScript(script);
        }
        if(JSVAL_IS_OBJECT(res)) {
          m_pJSThis = JSVAL_TO_OBJECT(res);
          m_pSelf.reset(new ScriptObject(m_pJSThis, *m_pContext));
          m_pSelf->addRoot();
        }
      } catch(ScriptException& e) {
        Logger::getInstance()->log(std::string("DSIDJS: Could not get 'self' object: ") + e.what());
      }
    } // createJSDevice

  private:
    boost::shared_ptr<ScriptContext> m_pContext;
    JSObject* m_pJSThis;
    boost::shared_ptr<ScriptObject> m_pSelf;
    const std::vector<std::string>& m_FileNames;
  }; // DSIDJS

  //================================================== DSIDJSCreator

  DSIDJSCreator::DSIDJSCreator(const std::vector<std::string>& _fileNames, const std::string& _pluginName, DSSim& _simulator)
  : DSIDCreator(_pluginName),
    m_pScriptEnvironment(new ScriptEnvironment()),
    m_FileNames(_fileNames),
    m_Simulator(_simulator)
  {
    m_pScriptEnvironment->initialize();
    m_pScriptEnvironment->addExtension(new PropertyScriptExtension(DSS::getInstance()->getPropertySystem()));
    m_pScriptEnvironment->addExtension(new ModelConstantsScriptExtension());
    m_pScriptEnvironment->addExtension(new SocketScriptContextExtension());
#if HAVE_CURL
    m_pScriptEnvironment->addExtension(new CurlScriptContextExtension());
#endif
  } // ctor

  DSIDInterface* DSIDJSCreator::createDSID(const dss_dsid_t _dsid, const devid_t _shortAddress, const DSMeterSim& _dsMeter) {
    boost::shared_ptr<ScriptContext> pContext(m_pScriptEnvironment->getContext());
    DSIDJS* result = new DSIDJS(_dsMeter, _dsid, _shortAddress, pContext, m_FileNames);
    return result;
  } // createDSID


} // namespace dss
