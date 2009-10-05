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

#include "dsid_js.h"
#include "core/jshandler.h"
#include "core/scripting/modeljs.h"
#include "core/dss.h"
#include "core/thread.h"

namespace dss {

  class DSIDJS : public DSIDInterface {
  public:
    DSIDJS(const DSModulatorSim& _simulator, dsid_t _dsid,
           devid_t _shortAddress, boost::shared_ptr<ScriptContext> _pContext,
           const std::string& _fileName)
    : DSIDInterface(_simulator, _dsid, _shortAddress),
      m_pContext(_pContext),
      m_FileName(_fileName)
    {}

    virtual ~DSIDJS() {}

    virtual void initialize() {
      try {
        jsval res = m_pContext->evaluateScript<jsval>(m_FileName);
        if(JSVAL_IS_OBJECT(res)) {
          m_pJSThis = JSVAL_TO_OBJECT(res);
          m_pSelf.reset(new ScriptObject(m_pJSThis, *m_pContext));
          ScriptFunctionParameterList param(*m_pContext);
          param.add(getDSID().toString());
          param.add(getShortAddress());
          param.add(getZoneID());
          try {
            m_pSelf->callFunctionByName<void>("initialize", param);
          } catch(ScriptException& e) {
            Logger::getInstance()->log(std::string("DSIDJS: Error calling 'initialize'") + e.what(), lsError);
          }
        }
      } catch(ScriptException& e) {
        Logger::getInstance()->log("DSIDJS: Could not get 'self' object");
      }
    }

    virtual void callScene(const int _sceneNr) {
      if(m_pSelf != NULL) {
        try {
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
          ScriptFunctionParameterList param(*m_pContext);
          param.add(_sceneNr);
          m_pSelf->callFunctionByName<void>("undoScene", param);
        } catch(ScriptException& e) {
          Logger::getInstance()->log(std::string("DSIDJS: Error calling 'undoScene'") + e.what(), lsError);
        }
      }
    } // undoScene

    virtual void increaseValue(const int _parameterNr = -1) {
      if(m_pSelf != NULL) {
        try {
          ScriptFunctionParameterList param(*m_pContext);
          m_pSelf->callFunctionByName<void>("increaseValue", param);
        } catch(ScriptException& e) {
          Logger::getInstance()->log(std::string("DSIDJS: Error calling 'increaseValue'") + e.what(), lsError);
        }
      }
    } // increaseValue

    virtual void decreaseValue(const int _parameterNr = -1) {
      if(m_pSelf != NULL) {
        try {
          ScriptFunctionParameterList param(*m_pContext);
          m_pSelf->callFunctionByName<void>("decreaseValue", param);
        } catch(ScriptException& e) {
          Logger::getInstance()->log(std::string("DSIDJS: Error calling 'decreaseValue'") + e.what(), lsError);
        }
      }
    } // decreaseValue

    virtual void enable() {
      if(m_pSelf != NULL) {
        try {
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
          ScriptFunctionParameterList param(*m_pContext);
          m_pSelf->callFunctionByName<void>("disable", param);
        } catch(ScriptException& e) {
          Logger::getInstance()->log(std::string("DSIDJS: Error calling 'disable'") + e.what(), lsError);
        }
      }
    } // disable

    virtual int getConsumption() {
      if(m_pSelf != NULL) {
        try {
          ScriptFunctionParameterList param(*m_pContext);
          return m_pSelf->callFunctionByName<int>("getConsumption", param);
        } catch(ScriptException& e) {
          Logger::getInstance()->log(std::string("DSIDJS: Error calling 'getConsumption'") + e.what(), lsError);
        }
      }
      return 0;
    } // getConsumption

    virtual void startDim(bool _directionUp, const int _parameterNr = -1) {
      if(m_pSelf != NULL) {
        try {
          ScriptFunctionParameterList param(*m_pContext);
          param.add(_directionUp);
          m_pSelf->callFunctionByName<void>("startDim", param);
        } catch(ScriptException& e) {
          Logger::getInstance()->log(std::string("DSIDJS: Error calling 'startDim'") + e.what(), lsError);
        }
      }
    } // startDim

    virtual void endDim(const int _parameterNr = -1) {
      if(m_pSelf != NULL) {
        try {
          ScriptFunctionParameterList param(*m_pContext);
          m_pSelf->callFunctionByName<void>("endDim", param);
        } catch(ScriptException& e) {
          Logger::getInstance()->log(std::string("DSIDJS: Error calling 'endDim'") + e.what(), lsError);
        }
      }
    } // endDim

    virtual void setValue(const double _value, int _parameterNr = -1) {
      if(m_pSelf != NULL) {
        try {
          ScriptFunctionParameterList param(*m_pContext);
          param.add(int(_value));
          param.add(_parameterNr);
          m_pSelf->callFunctionByName<void>("setValue", param);
        } catch(ScriptException& e) {
          Logger::getInstance()->log(std::string("DSIDJS: Error calling 'setValue'") + e.what(), lsError);
        }
      }
    } // setValue

    virtual double getValue(int _parameterNr = -1) const {
      if(m_pSelf != NULL) {
        try {
          ScriptFunctionParameterList param(*m_pContext);
          param.add(_parameterNr);
          return m_pSelf->callFunctionByName<int>("getValue", param);
        } catch(ScriptException& e) {
          Logger::getInstance()->log(std::string("DSIDJS: Error calling 'getValue'") + e.what(), lsError);
        }
      }
      return 0;
    } // getValue

    virtual uint16_t getFunctionID() {
      if(m_pSelf != NULL) {
        try {
          ScriptFunctionParameterList param(*m_pContext);
          return m_pSelf->callFunctionByName<int>("getFunctionID", param);
        } catch(ScriptException& e) {
          Logger::getInstance()->log(std::string("DSIDJS: Error calling 'getFunctionID'") + e.what(), lsError);
        }
      }
      return 0;
    } // getFunctionID

    virtual void setConfigParameter(const string& _name, const string& _value) {
      if(m_pSelf != NULL) {
        try {
          ScriptFunctionParameterList param(*m_pContext);
          param.add(_name);
          param.add(_value);
          m_pSelf->callFunctionByName<void>("setConfigParameter", param);
        } catch(ScriptException& e) {
          Logger::getInstance()->log(std::string("DSIDJS: Error calling 'setConfigParameter'") + e.what(), lsError);
        }
      }
    } // setConfigParameter

    virtual string getConfigParameter(const string& _name) const {
      if(m_pSelf != NULL) {
        try {
          ScriptFunctionParameterList param(*m_pContext);
          param.add(_name);
          return m_pSelf->callFunctionByName<std::string>("getConfigParameter", param);
        } catch(ScriptException& e) {
          Logger::getInstance()->log(std::string("DSIDJS: Error calling 'getConfigParameter'") + e.what(), lsError);
        }
      }
      return "";
    } // getConfigParameter

    virtual uint8_t dsLinkSend(uint8_t _value, uint8_t _flags, bool& _handled) {
      if(m_pSelf != NULL) {
        try {
          ScriptFunctionParameterList param(*m_pContext);
          param.add(int(_value));
          param.add(int(_flags));
          int res = m_pSelf->callFunctionByName<int>("dSLinkSend", param);
          _handled = true;
          return res;
        } catch(ScriptException& e) {
          Logger::getInstance()->log(std::string("DSIDJS: Error calling 'dSLinkSend'") + e.what(), lsError);
        }
        _handled = false;
        return 0;
      }
      return 0;
    } // dsLinkSend
  private:
    boost::shared_ptr<ScriptContext> m_pContext;
    JSObject* m_pJSThis;
    boost::shared_ptr<ScriptObject> m_pSelf;
    std::string m_FileName;
  }; // DSIDJS


  //================================================== DSIDScriptExtension

  const char* DSIDScriptExtensionName = "dsidextension";

  class DSIDScriptExtension : public ScriptExtension {
  public:
    DSIDScriptExtension(DSSim& _simulation)
    : ScriptExtension(DSIDScriptExtensionName),
      m_Simulation(_simulation)
    { } // ctor

    virtual ~DSIDScriptExtension() {}

    virtual void extendContext(ScriptContext& _context);

    void dSLinkInterrupt(const dsid_t& _dsid) {
      DSIDInterface* intf = m_Simulation.getSimulatedDevice(_dsid);
      if(intf != NULL) {
        intf->dSLinkInterrupt();
      }
    } // dSLinkInterrupt

  private:
    DSSim& m_Simulation;
  }; // PropertyScriptExtension

  class DSLinkInterrupSender : public Thread {
  public:
    DSLinkInterrupSender(dsid_t _dsid, DSIDScriptExtension* _ext)
    : Thread("DSLinkInterruptSender"),
      m_DSID(_dsid), m_Ext(_ext)
    {
      setFreeAtTermination(true);
    }
    virtual void execute() {
      sleepMS(rand() % 3000);
      m_Ext->dSLinkInterrupt(m_DSID);
    }
  private:
    dsid_t m_DSID;
    DSIDScriptExtension* m_Ext;
  };

  JSBool global_dsid_dSLinkInterrupt(JSContext* cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    if(argc < 1) {
      Logger::getInstance()->log("JS: glogal_dsid_dSLinkInterrupt: need argument dsid", lsError);
    } else {
      ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

      DSIDScriptExtension* ext = dynamic_cast<DSIDScriptExtension*>(ctx->getEnvironment().getExtension(DSIDScriptExtensionName));
      std::string dsidString = ctx->convertTo<std::string>(argv[0]);

      try {
        dsid_t dsid = dsid_t::fromString(dsidString);
        DSLinkInterrupSender* sender = new DSLinkInterrupSender(dsid, ext);
        sender->run();
      } catch(std::runtime_error&) {
        Logger::getInstance()->log("Could not parse DSID");
      }

      *rval = JSVAL_TRUE;
      return JS_TRUE;
    }
    return JS_FALSE;
  } // global_prop_setListener

  JSFunctionSpec dsid_global_methods[] = {
    {"dSLinkInterrupt", global_dsid_dSLinkInterrupt, 1, 0, 0},
    {NULL},
  };

  void DSIDScriptExtension::extendContext(ScriptContext& _context) {
    JS_DefineFunctions(_context.getJSContext(), JS_GetGlobalObject(_context.getJSContext()), dsid_global_methods);
  } // extendContext


  //================================================== DSIDJSCreator

  DSIDJSCreator::DSIDJSCreator(const std::string& _fileName, const std::string& _pluginName, DSSim& _simulator)
  : DSIDCreator(_pluginName),
    m_pScriptEnvironment(new ScriptEnvironment()),
    m_FileName(_fileName),
    m_Simulator(_simulator)
  {
    m_pScriptEnvironment->initialize();
    m_pScriptEnvironment->addExtension(new PropertyScriptExtension(DSS::getInstance()->getPropertySystem()));
    m_pScriptEnvironment->addExtension(new DSIDScriptExtension(m_Simulator));
  } // ctor

  DSIDInterface* DSIDJSCreator::createDSID(const dsid_t _dsid, const devid_t _shortAddress, const DSModulatorSim& _modulator) {
    boost::shared_ptr<ScriptContext> pContext(m_pScriptEnvironment->getContext());
    DSIDJS* result = new DSIDJS(_modulator, _dsid, _shortAddress, pContext, m_FileName);
    return result;
  } // createDSID


} // namespace dss
