/*
    Copyright (c) 2011 digitalSTROM.org, Zurich, Switzerland

    Authors: Michael Tross, aizo GmbH <michael.tross@aizo.com>
             Christian Hitz, aizo AG <christian.hitz@aizo.com>

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

#include <sstream>
#include <boost/make_shared.hpp>

#include <digitalSTROM/dsuid.h>


#include "jsdatabase.h"
#include "sqlite3_wrapper.h"
#include "scriptobject.h"
#include "security/security.h"
#include "stringconverter.h"
#include "dss.h"

namespace dss {

  const std::string DatabaseScriptExtensionName = "databaseextension";

  JSBool database_query(JSContext* cx, uintN argc, jsval *vp) {
    ScriptContext* ctx = static_cast<ScriptContext*>(JS_GetContextPrivate(cx));

    try {
      if(argc < 1) {
          JS_ReportError(cx, "query(): nothing to query - missing parameter",
                         lsError);
        return JS_FALSE;
      }

      std::string sql;

      if (JSVAL_IS_STRING(JS_ARGV(cx, vp)[0])) {
        StringConverter st("UTF-8", "UTF-8");
        sql = st.convert(ctx->convertTo<std::string>(JS_ARGV(cx, vp)[0]));
      } else {
        JS_ReportError(cx, "query(): wrong parameter type", lsError);
        return JS_FALSE;
      }

      if (sql.empty()) {
        JS_ReportError(cx, "query(): empty query parameter", lsError);
        return JS_FALSE;
      }

      std::string id = ctx->getWrapper()->getIdentifier();

      std::string database = DSS::getInstance()->getDatabaseDirectory() + id +
                             ".db";

      boost::shared_ptr<SQLite3> sqlite = boost::make_shared<SQLite3>(database);
      boost::shared_ptr<SQLite3::query_result> q = sqlite->query(sql);

      JSObject* resultObj = JS_NewArrayObject(cx, 0, NULL);
      JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(resultObj));

      for (size_t i = 0; i < q->size(); i++) {
        boost::shared_ptr<SQLite3::row_result> rr = q->at(i);
        // row
        ScriptObject rowObj(*ctx, NULL);
        jsval rowVal = OBJECT_TO_JSVAL(rowObj.getJSObject());
        JSBool res = JS_SetElement(cx, resultObj, i, &rowVal);
        if (!res) {
          JS_ReportError(cx, "query(): could not add element", lsError);
          return JS_FALSE;
        }

        for (size_t j = 0; j < rr->size(); j++) {
          boost::shared_ptr<SQLite3::cell> cell = rr->at(j);
          rowObj.setProperty<std::string>(cell->name, cell->data);
        }
      }

      return JS_TRUE;
    } catch(ScriptException& e) {
      JS_ReportError(cx, "Scripting failure: %s", e.what());
    } catch (SecurityException& ex) {
      JS_ReportError(cx, "Access denied: %s", ex.what());
    } catch (DSSException& ex) {
      JS_ReportError(cx, "Failure: %s", ex.what());
    } catch (std::exception& ex) {
      JS_ReportError(cx, "General failure: %s", ex.what());
    }
    return JS_FALSE;
  } // metering_getAggregatedValues


  JSFunctionSpec database_static_methods[] = {
    JS_FS("query", database_query, 1, 0),
    JS_FS_END
  };

  static JSClass database_class = {
    "SQL", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,  JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStandardClasses,
    JS_ResolveStub,
    JS_ConvertStub,
    JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
  }; // database_class

  JSBool database_construct(JSContext *cx, uintN argc, jsval *vp) {
    ScriptContext *ctx = static_cast<ScriptContext *>(JS_GetContextPrivate(cx));
    JSRequest req(ctx);
    JSObject *obj = JS_NewObject(cx, &database_class, NULL, NULL);
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(obj));
    return JS_TRUE;
  } // database_construct

  DatabaseScriptExtension::DatabaseScriptExtension()
  : ScriptExtension(DatabaseScriptExtensionName)
  { }
 
  void DatabaseScriptExtension::extendContext(ScriptContext& _context) {
    JS_InitClass(_context.getJSContext(),
                 _context.getRootObject().getJSObject(),
                 NULL, /* protoype */
                 &database_class, /* class */
                 &database_construct, 0, /* constructor + args */
                 NULL, /* properties */
                 database_static_methods, /* functions */
                 NULL, /* static_ps */
                 NULL); /* static_fs */
  } // extendedJSContext

} // namespace
