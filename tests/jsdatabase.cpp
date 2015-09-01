#define BOOST_TEST_NO_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <boost/scoped_ptr.hpp>
#include <boost/filesystem.hpp>
#include <memory>

#include "src/dss.h"
#include "scripting/scriptobject.h"
#include "scripting/jsdatabase.h"
#include "tests/util/dss_instance_fixture.h"

using namespace std;
using namespace dss;

BOOST_FIXTURE_TEST_SUITE(JSDatabase, DSSInstanceFixture)

BOOST_FIXTURE_TEST_CASE(testDatabase, DSSInstanceFixture) {

  PropertySystem &propSys = DSS::getInstance()->getPropertySystem();
  
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  DatabaseScriptExtension* ext = new DatabaseScriptExtension();
  env->addExtension(ext);
 
  std::string script_id = "jsdatabasetest";

  boost::shared_ptr<ScriptContext> ctx(env->getContext());
  boost::shared_ptr<ScriptContextWrapper> wrapper
    (new ScriptContextWrapper(ctx, propSys.getRootNode(), script_id, true));
  ctx->attachWrapper(wrapper);

  std::string db = DSS::getInstance()->getDatabaseDirectory() + script_id + ".db";
  if (!db.empty()) {
    if (boost::filesystem::exists(db)) {
      boost::filesystem::remove_all(db);
    }
  }

  int entries = ctx->evaluate<int>("var sql = new SQL();"
    "sql.query(\"CREATE TABLE led(id PRIMARY KEY, name, value)\");"
    "sql.query(\"INSERT INTO led VALUES(1, \\\"aaa\\\", \\\"valueA\\\")\");"
    "sql.query(\"INSERT INTO led VALUES(2, \\\"bbb\\\", \\\"valueB\\\")\");"
    "response = sql.query(\"SELECT * FROM led\");"
    "print(uneval(response));"
    "response.length;" 
  );

  if (!db.empty()) {
    boost::filesystem::remove_all(db);
  }

  BOOST_CHECK_EQUAL(entries, 2);
}

BOOST_AUTO_TEST_SUITE_END()
