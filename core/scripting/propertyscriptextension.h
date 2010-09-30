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

#ifndef PROPERTYSCRIPTEXTENSION_H_
#define PROPERTYSCRIPTEXTENSION_H_

#include "core/jshandler.h"
#include "core/propertysystem.h"

namespace dss {

  class PropertyScriptExtension;

  class PropertyScriptListener : public PropertyListener,
                                 public ScriptContextAttachedObject {
  public:
    PropertyScriptListener(PropertyScriptExtension* _pExtension, ScriptContext* _pContext, JSObject* _functionObj, jsval _function, const std::string& _identifier);
    virtual ~PropertyScriptListener();

    virtual void propertyChanged(PropertyNodePtr _caller, PropertyNodePtr _changedNode);
    virtual void propertyRemoved(PropertyNodePtr _parent, PropertyNodePtr _child);
    virtual void propertyAdded(PropertyNodePtr _parent, PropertyNodePtr _child);

    const std::string& getIdentifier() const { return m_Identifier; }
  private:
    void doOnChange(PropertyNodePtr _changedNode);
    void createScriptObject();
  private:
    PropertyScriptExtension* m_pExtension;
    JSObject* m_pFunctionObject;
    jsval m_Function;
    const std::string m_Identifier;
    boost::scoped_ptr<ScriptObject> m_pScriptObject;
    ScriptFunctionRooter m_FunctionRoot;
  }; // PropertyScriptListener

  class PropertyScriptExtension : public ScriptExtension {
  public:
    PropertyScriptExtension(PropertySystem& _propertySystem);
    virtual ~PropertyScriptExtension();

    virtual void extendContext(ScriptContext& _context);

    PropertySystem& getPropertySystem() { return m_PropertySystem; }

    JSObject* createJSProperty(ScriptContext& _ctx, boost::shared_ptr<PropertyNode> _node);
    std::string produceListenerID();
    void addListener(PropertyScriptListener* _pListener);
    void removeListener(const std::string& _identifier, bool _destroy = true);
  private:
    PropertySystem& m_PropertySystem;
    std::vector<PropertyScriptListener*> m_Listeners;
    int m_NextListenerID;
  }; // PropertyScriptExtension

} // namespace dss

#endif /* PROPERTYSCRIPTEXTENSION_H_ */
