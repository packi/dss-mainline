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

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif


#include "propertyrequesthandler.h"

#include "src/propertysystem.h"

#include "src/propertyquery.h"
#include "src/stringconverter.h"
#include "util.h"

namespace dss {


  //=========================================== PropertyRequestHandler

  PropertyRequestHandler::PropertyRequestHandler(PropertySystem& _propertySystem)
  : m_PropertySystem(_propertySystem)
  { }

  WebServerResponse PropertyRequestHandler::jsonHandleRequest(const RestfulRequest& _request, boost::shared_ptr<Session> _session) {
    StringConverter st("UTF-8", "UTF-8");

    if (!DSS::getInstance()->drainModelChanges()) {
        return JSONWriter::failure("Failed to access data model");
    }

    if (_request.getMethod() == "query" || _request.getMethod() == "query2" || _request.getMethod() == "vdcquery") {
      JSONWriter json;
      std::string query = _request.getParameter("query");
      if(query.empty()) {
        return JSONWriter::failure("Need parameter 'query'");
      }

      if (_request.getMethod() == "vdcquery") {
        PropertyQuery propertyQuery(m_PropertySystem.getRootNode(), query, false);
        propertyQuery.vdcquery(json);
        return json.successJSON();
      }
      else if (_request.getMethod() == "query2") {
        PropertyQuery propertyQuery(m_PropertySystem.getRootNode(), query, true);
        propertyQuery.run2(json);
        return json.successJSON();
      }
      PropertyQuery propertyQuery(m_PropertySystem.getRootNode(), query, true);
      propertyQuery.run(json);
      return json.successJSON();
    }

    std::string propName = st.convert(_request.getParameter("path"));
    if(propName.empty()) {
      return JSONWriter::failure("Need parameter 'path' for property operations");
    }
    propName = escapeHTML(propName);
    PropertyNodePtr node = m_PropertySystem.getProperty(propName);

    if(_request.getMethod() == "remove") {
      if(node == NULL) {
        return JSONWriter::failure("Could not find node named '" + propName + "'");
      }
      try {
        PropertyNode* parent = node->getParentNode();
        if(parent != NULL) {
          parent->removeChild(node);
          return JSONWriter::success();
        } else {
          return JSONWriter::failure("Can't remove root node");
        }
      } catch (...) {
        return JSONWriter::failure(std::string("Error removing node '") + propName + "'");
      }
    } else if(_request.getMethod() == "getString") {
      if(node == NULL) {
        return JSONWriter::failure("Could not find node named '" + propName + "'");
      }
      try {
        JSONWriter json;
        json.add("value", node->getStringValue());
        return json.successJSON();
      } catch(PropertyTypeMismatch& ex) {
        return JSONWriter::failure(std::string("Error getting property: '") + ex.what() + "'");
      }
    } else if(_request.getMethod() == "getInteger") {
      if(node == NULL) {
        return JSONWriter::failure("Could not find node named '" + propName + "'");
      }

      try {
        JSONWriter json;
        if (node->getValueType() == vTypeInteger) {
          json.add("value", node->getIntegerValue());
        } else {
          json.add("value", node->getUnsignedIntegerValue());
        }
        return json.successJSON();
      } catch(PropertyTypeMismatch& ex) {
        return JSONWriter::failure(std::string("Error getting property: '") + ex.what() + "'");
      }
    } else if(_request.getMethod() == "getBoolean") {
      if(node == NULL) {
        return JSONWriter::failure("Could not find node named '" + propName + "'");
      }
      try {
        JSONWriter json;
        json.add("value", node->getBoolValue());
        return json.successJSON();
      } catch(PropertyTypeMismatch& ex) {
        return JSONWriter::failure(std::string("Error getting property: '") + ex.what() + "'");
      }
    } else if(_request.getMethod() == "getFloating") {
      if(node == NULL) {
        return JSONWriter::failure("Could not find node named '" + propName + "'");
      }
      try {
        JSONWriter json;
        json.add("value", node->getFloatingValue());
        return json.successJSON();
      } catch(PropertyTypeMismatch& ex) {
        return JSONWriter::failure(std::string("Error getting property: '") + ex.what() + "'");
      }
    } else if(_request.getMethod() == "setString") {
      std::string value = st.convert(_request.getParameter("value"));

      value = escapeHTML(value);

      if(node == NULL) {
        node = m_PropertySystem.createProperty(propName);
      }
      try {
        node->setStringValue(value);
      } catch(PropertyTypeMismatch& ex) {
        return JSONWriter::failure(std::string("Error setting property: '") + ex.what() + "'");
      }
      return JSONWriter::success();
    } else if(_request.getMethod() == "setBoolean") {
      std::string strValue = _request.getParameter("value");
      bool value;
      if(strValue == "true") {
        value = true;
      } else if(strValue == "false") {
        value = false;
      } else {
        return JSONWriter::failure("Expected 'true' or 'false' for parameter 'value' but got: '" + strValue + "'");
      }
      if(node == NULL) {
        node = m_PropertySystem.createProperty(propName);
      }
      try {
        node->setBooleanValue(value);
      } catch(PropertyTypeMismatch& ex) {
        return JSONWriter::failure(std::string("Error setting property: '") + ex.what() + "'");
      }
      return JSONWriter::success();
    } else if(_request.getMethod() == "setInteger") {
      aValueType type = vTypeInteger;
      if(node == NULL) {
        node = m_PropertySystem.createProperty(propName);
      } else {
        type = node->getValueType();
      }
      std::string strValue = _request.getParameter("value");
      int value;
      uint32_t uvalue;

      if (type == vTypeInteger) {
        try {
          value = strToInt(strValue);
        } catch(...) {
          return JSONWriter::failure("Could not convert parameter 'value' to integer. Got: '" + strValue + "'");
        }
        try {
          node->setIntegerValue(value);
        } catch(PropertyTypeMismatch& ex) {
          return JSONWriter::failure(std::string("Error setting property: '") + ex.what() + "'");
        }
      } else {
        try {
          uvalue = strToUInt(strValue);
        } catch(...) {
          return JSONWriter::failure("Could not convert parameter 'value' to integer. Got: '" + strValue + "'");
        }
        try {
          node->setUnsignedIntegerValue(uvalue);
        } catch(PropertyTypeMismatch& ex) {
          return JSONWriter::failure(std::string("Error setting property: '") + ex.what() + "'");
        }
      }
      return JSONWriter::success();
    } else if(_request.getMethod() == "setFloating") {
      std::string strValue = _request.getParameter("value");
      int value;
      try {
        value = strToDouble(strValue);
      } catch(...) {
        return JSONWriter::failure("Could not convert parameter 'value' to std::string. Got: '" + strValue + "'");
      }
      if(node == NULL) {
        node = m_PropertySystem.createProperty(propName);
      }
      try {
        node->setFloatingValue(value);
      } catch(PropertyTypeMismatch& ex) {
        return JSONWriter::failure(std::string("Error setting property: '") + ex.what() + "'");
      }
      return JSONWriter::success();
    } else if(_request.getMethod() == "getChildren") {
      if(node == NULL) {
        return JSONWriter::failure("Could not find node named '" + propName + "'");
      }
      JSONWriter json(JSONWriter::jsonArrayResult);
      for(int iChild = 0; iChild < node->getChildCount(); iChild++) {
        json.startObject();
        PropertyNodePtr cnode = node->getChild(iChild);
        json.add("name", cnode->getDisplayName());
        // do not expose unsigned int to the UI
        if (cnode->getValueType() == vTypeUnsignedInteger) {
          json.add("type", getValueTypeAsString(vTypeInteger));
        } else {
          json.add("type", getValueTypeAsString(cnode->getValueType()));
        }
        json.endObject();
      }
      return json.successJSON();
    } else if(_request.getMethod() == "getType") {
      if(node == NULL) {
        return JSONWriter::failure("Could not find node named '" + propName + "'");
      }
      JSONWriter json;
      json.add("type", getValueTypeAsString(node->getValueType()));
      return json.successJSON();
    } else if(_request.getMethod() == "setFlag") {
      PropertyNode::Flag flag;
      bool value;

      if(node == NULL) {
        return JSONWriter::failure("Could not find node named '" + propName + "'");
      }

      std::string strFlag = _request.getParameter("flag");
      if(strFlag == "READABLE") {
        flag = PropertyNode::Readable;
      } else if(strFlag == "WRITEABLE") {
        flag = PropertyNode::Writeable;
      } else if(strFlag == "ARCHIVE") {
        flag = PropertyNode::Archive;
      } else {
        return JSONWriter::failure("Expected 'READABLE', 'WRITEABLE' or 'ARCHIVE' for parameter 'flag' but got: '" + strFlag + "'");
      }

      std::string strValue = _request.getParameter("value");
      if(strValue == "true") {
        value = true;
      } else if(strValue == "false") {
        value = false;
      } else {
        return JSONWriter::failure("Expected 'true' or 'false' for parameter 'value' but got: '" + strValue + "'");
      }

      node->setFlag(flag, value);

      return JSONWriter::success();
    } else if(_request.getMethod() == "getFlags") {
      if(node == NULL) {
        return JSONWriter::failure("Could not find node named '" + propName + "'");
      }

      JSONWriter json;
      json.add("READABLE", node->hasFlag(PropertyNode::Readable));
      json.add("WRITEABLE", node->hasFlag(PropertyNode::Writeable));
      json.add("ARCHIVE", node->hasFlag(PropertyNode::Archive));

      return json.successJSON();
    }
    throw std::runtime_error("Unhandled function");
  } // handleRequest

} // namespace dss
