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

#include "propertyrequesthandler.h"

#include "core/propertysystem.h"

#include "core/web/json.h"
#include "core/propertyquery.h"

namespace dss {


  //=========================================== PropertyRequestHandler
  
  PropertyRequestHandler::PropertyRequestHandler(PropertySystem& _propertySystem)
  : m_PropertySystem(_propertySystem)
  { }

  WebServerResponse PropertyRequestHandler::jsonHandleRequest(const RestfulRequest& _request, boost::shared_ptr<Session> _session) {
    if(_request.getMethod() == "query") {
      std::string query = _request.getParameter("query");
      if(query.empty()) {
        return failure("Need parameter 'query'");
      }
      PropertyQuery propertyQuery(m_PropertySystem.getRootNode(), query);
      return success(propertyQuery.run());
    }

    std::string propName = _request.getParameter("path");
    if(propName.empty()) {
      return failure("Need parameter 'path' for property operations");
    }
    PropertyNodePtr node = m_PropertySystem.getProperty(propName);

    if(_request.getMethod() == "getString") {
      if(node == NULL) {
        return failure("Could not find node named '" + propName + "'");
      }
      try {
        boost::shared_ptr<JSONObject> resultObj(new JSONObject());
        resultObj->addProperty("value", node->getStringValue());
        return success(resultObj);
      } catch(PropertyTypeMismatch& ex) {
        return failure(std::string("Error getting property: '") + ex.what() + "'");
      }
    } else if(_request.getMethod() == "getInteger") {
      if(node == NULL) {
        return failure("Could not find node named '" + propName + "'");
      }
      try {
        boost::shared_ptr<JSONObject> resultObj(new JSONObject());
        resultObj->addProperty("value", node->getIntegerValue());
        return success(resultObj);
      } catch(PropertyTypeMismatch& ex) {
        return failure(std::string("Error getting property: '") + ex.what() + "'");
      }
    } else if(_request.getMethod() == "getBoolean") {
      if(node == NULL) {
        return failure("Could not find node named '" + propName + "'");
      }
      try {
        boost::shared_ptr<JSONObject> resultObj(new JSONObject());
        resultObj->addProperty("value", node->getBoolValue());
        return success(resultObj);
      } catch(PropertyTypeMismatch& ex) {
        return failure(std::string("Error getting property: '") + ex.what() + "'");
      }
    } else if(_request.getMethod() == "setString") {
      std::string value = _request.getParameter("value");
      if(node == NULL) {
        node = m_PropertySystem.createProperty(propName);
      }
      try {
        node->setStringValue(value);
      } catch(PropertyTypeMismatch& ex) {
        return failure(std::string("Error setting property: '") + ex.what() + "'");
      }
      return success();
    } else if(_request.getMethod() == "setBoolean") {
      std::string strValue = _request.getParameter("value");
      bool value;
      if(strValue == "true") {
        value = true;
      } else if(strValue == "false") {
        value = false;
      } else {
        return failure("Expected 'true' or 'false' for parameter 'value' but got: '" + strValue + "'");
      }
      if(node == NULL) {
        node = m_PropertySystem.createProperty(propName);
      }
      try {
        node->setBooleanValue(value);
      } catch(PropertyTypeMismatch& ex) {
        return failure(std::string("Error setting property: '") + ex.what() + "'");
      }
      return success();
    } else if(_request.getMethod() == "setInteger") {
      std::string strValue = _request.getParameter("value");
      int value;
      try {
        value = strToInt(strValue);
      } catch(...) {
        return failure("Could not convert parameter 'value' to std::string. Got: '" + strValue + "'");
      }
      if(node == NULL) {
        node = m_PropertySystem.createProperty(propName);
      }
      try {
        node->setIntegerValue(value);
      } catch(PropertyTypeMismatch& ex) {
        return failure(std::string("Error setting property: '") + ex.what() + "'");
      }
      return success();
    } else if(_request.getMethod() == "getChildren") {
      if(node == NULL) {
        return failure("Could not find node named '" + propName + "'");
      }
      boost::shared_ptr<JSONArrayBase> resultObj(new JSONArrayBase());
      for(int iChild = 0; iChild < node->getChildCount(); iChild++) {
        boost::shared_ptr<JSONObject> prop(new JSONObject());
        resultObj->addElement("", prop);
        PropertyNodePtr cnode = node->getChild(iChild);
        prop->addProperty("name", cnode->getName());
        prop->addProperty("type", getValueTypeAsString(cnode->getValueType()));
      }
      return success(resultObj);
    } else if(_request.getMethod() == "getType") {
      if(node == NULL) {
        return failure("Could not find node named '" + propName + "'");
      }
      boost::shared_ptr<JSONObject> resultObj(new JSONObject());
      resultObj->addProperty("type", getValueTypeAsString(node->getValueType()));
      return success(resultObj);
    } else if(_request.getMethod() == "setFlag") {
      PropertyNode::Flag flag;
      bool value;

      if(node == NULL) {
        return failure("Could not find node named '" + propName + "'");
      }

      std::string strFlag = _request.getParameter("flag");
      if(strFlag == "READABLE") {
        flag = PropertyNode::Readable;
      } else if(strFlag == "WRITEABLE") {
        flag = PropertyNode::Writeable;
      } else if(strFlag == "ARCHIVE") {
        flag = PropertyNode::Archive;
      } else {
        return failure("Expected 'READABLE', 'WRITABLE' or 'ARCHIVE' for parameter 'flag' but got: '" + strFlag + "'");
      }

      std::string strValue = _request.getParameter("value");
      if(strValue == "true") {
        value = true;
      } else if(strValue == "false") {
        value = false;
      } else {
        return failure("Expected 'true' or 'false' for parameter 'value' but got: '" + strValue + "'");
      }

      node->setFlag(flag, value);

      return success();
    } else if(_request.getMethod() == "getFlags") {
      if(node == NULL) {
        return failure("Could not find node named '" + propName + "'");
      }

      boost::shared_ptr<JSONObject> resultObj(new JSONObject());
      resultObj->addProperty("READABLE", node->hasFlag(PropertyNode::Readable));
      resultObj->addProperty("WRITEABLE", node->hasFlag(PropertyNode::Writeable));
      resultObj->addProperty("ARCHIVE", node->hasFlag(PropertyNode::Archive));

      return success(resultObj);
    }
    throw std::runtime_error("Unhandled function");
  } // handleRequest

} // namespace dss
