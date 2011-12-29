/*
    Copyright (c) 2009,2011 digitalSTROM.org, Zurich, Switzerland

    Author: Patrick Staehlin, futureLAB AG <pstaehlin@futurelab.ch>,
            Michael Tross, aizo GmbH <michael.tross@aizo.com>

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

#ifndef PROPERTYSCRIPTEXTENSION_H_
#define PROPERTYSCRIPTEXTENSION_H_

#include <boost/enable_shared_from_this.hpp>

#include "src/scripting/jshandler.h"
#include "src/propertysystem.h"

namespace dss {

  class Security;
  class User;
  class PropertyScriptExtension;

  class PropertyScriptListener : public PropertyListener,
                                 public ScriptContextAttachedObject,
                                 public boost::enable_shared_from_this<PropertyScriptListener> {
  public:
    PropertyScriptListener(PropertyScriptExtension* _pExtension, ScriptContext* _pContext, JSObject* _functionObj, jsval _function, const std::string& _identifier);
    virtual ~PropertyScriptListener();

    virtual void propertyChanged(PropertyNodePtr _caller, PropertyNodePtr _changedNode);
    virtual void propertyRemoved(PropertyNodePtr _parent, PropertyNodePtr _child);
    virtual void propertyAdded(PropertyNodePtr _parent, PropertyNodePtr _child);

    const std::string& getIdentifier() const { return m_Identifier; }
    virtual void stop();
  private:
    void doOnChange(PropertyNodePtr _changedNode);
    void deferredCallback(PropertyNodePtr _changedNode, JSObject* _obj,
        jsval _function, ScriptFunctionRooter* _rooter, boost::shared_ptr<PropertyScriptListener> _self);
  private:
    boost::shared_ptr<PropertyScriptListener> m_pSelf;
    PropertyScriptExtension* m_pExtension;
    JSObject* m_pFunctionObject;
    jsval m_Function;
    const std::string m_Identifier;
    boost::shared_ptr<ScriptObject> m_pScriptObject;
    ScriptFunctionRooter* m_FunctionRoot;
    User* m_pRunAsUser;
    bool m_callbackRunning;
  }; // PropertyScriptListener

  class PropertyScriptExtension : public ScriptExtension {
  public:
    PropertyScriptExtension(PropertySystem& _propertySystem);
    virtual ~PropertyScriptExtension();

    virtual void extendContext(ScriptContext& _context);

    PropertySystem& getPropertySystem() { return m_PropertySystem; }

    std::string produceListenerID();
    boost::shared_ptr<PropertyScriptListener> getListener(const std::string& _identifier);
    void addListener(boost::shared_ptr<PropertyScriptListener> _pListener);
    void removeListener(const std::string& _identifier);

    PropertyNodePtr getPropertyFromObj(ScriptContext* _context, JSObject* _obj);
    virtual PropertyNodePtr getProperty(ScriptContext* _context, const std::string& _path);
    virtual PropertyNodePtr createProperty(ScriptContext* _context, const std::string& _name);
    JSObject* createJSProperty(ScriptContext& _ctx, PropertyNode* _node);
    JSObject* createJSProperty(ScriptContext& _ctx, PropertyNodePtr _node);
    JSObject* createJSProperty(ScriptContext& _ctx, const std::string& _path);
    virtual bool store(ScriptContext* _ctx);
    virtual bool load(ScriptContext* _ctx);
  protected:
    PropertySystem& m_PropertySystem;
  private:
    std::vector<boost::shared_ptr<PropertyScriptListener> > m_Listeners;
    int m_NextListenerID;
  }; // PropertyScriptExtension

} // namespace dss

#endif /* PROPERTYSCRIPTEXTENSION_H_ */
