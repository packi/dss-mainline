#define BOOST_TEST_NO_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <boost/scoped_ptr.hpp>
#include <memory>

#include "event.h" // Thread
#include "propertysystem.h"
#include "scripting/scriptobject.h"
#include "scripting/jsproperty.h"

using namespace std;
using namespace dss;

BOOST_AUTO_TEST_SUITE(JSProperty)

BOOST_AUTO_TEST_CASE(testProperties) {
  PropertySystem propSys;
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new PropertyScriptExtension(propSys);
  env->addExtension(ext);

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  ctx->evaluate<void>("Property.setProperty('/testing', 1)");
  BOOST_CHECK_EQUAL(ctx->evaluate<int>("Property.getProperty('/testing')"), 1);
  BOOST_CHECK_EQUAL(propSys.getIntValue("/testing"), 1);

  ctx->evaluate<void>("Property.setProperty('/testing', 2)");
  BOOST_CHECK_EQUAL(ctx->evaluate<int>("Property.getProperty('/testing')"), 2);

  /* preserve unsigned integer type */
  propSys.getProperty("/testing")->setUnsignedIntegerValue(3);
  BOOST_CHECK_EQUAL(ctx->evaluate<int>("Property.getProperty('/testing')"), 3);
  ctx->evaluate<void>("Property.setProperty('/testing', 4)");
  BOOST_CHECK_EQUAL(propSys.getProperty("/testing")->getValueType(),
                    vTypeUnsignedInteger);
}

BOOST_AUTO_TEST_CASE(testPropertiesStrings) {
  PropertySystem propSys;
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new PropertyScriptExtension(propSys);
  env->addExtension(ext);

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  ctx->evaluate<void>("Property.setProperty('/testing', '<a href=\"test&\">\\\'bla\\\'</a>')");
  /* string should be HTML escaped */
  BOOST_CHECK_EQUAL(propSys.getStringValue("/testing"), "&lt;a href=&quot;test&amp;&quot;&gt;&#039;bla&#039;&lt;/a&gt;");
  /* getProperty in JS automatically unescapes HTML */
  BOOST_CHECK_EQUAL(ctx->evaluate<std::string>("Property.getProperty('/testing')"), "<a href=\"test&\">'bla'</a>");
}

BOOST_AUTO_TEST_CASE(testPropertyListener) {
  PropertySystem propSys;
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new PropertyScriptExtension(propSys);
  env->addExtension(ext);

  boost::shared_ptr<ScriptContext> ctx(env->getContext());
  ctx->evaluate<void>("Property.setProperty('/testing', 1); Property.setProperty('/triggered', false); "
                      "var listener_ident = Property.setListener('/testing', function(changedNode) { Property.setProperty('/triggered', true); }); "
      );

  BOOST_CHECK_EQUAL(propSys.getBoolValue("/triggered"), false);
  BOOST_CHECK_EQUAL(propSys.getIntValue("/testing"), 1);

  propSys.setIntValue("/testing", 2);
  sleep(1); // wait until listeners have run

  BOOST_CHECK_EQUAL(propSys.getBoolValue("/triggered"), true);
  BOOST_CHECK_EQUAL(propSys.getIntValue("/testing"), 2);

  // check that removing works
  ctx->evaluate<void>("Property.removeListener(listener_ident);");

  propSys.setBoolValue("/triggered", false);
  sleep(1); // wait until listeners have run

  BOOST_CHECK_EQUAL(propSys.getBoolValue("/triggered"), false);

  propSys.setIntValue("/testing", 2);
  sleep(1); // wait until listeners have run

  BOOST_CHECK_EQUAL(propSys.getBoolValue("/triggered"), false);
  BOOST_CHECK_EQUAL(propSys.getIntValue("/testing"), 2);

  // check that closures are working as expected
  ctx->evaluate<void>("Property.setProperty('/triggered', false); Property.setProperty('/testing', 1); "
                      "var ident = Property.setListener('/testing', function(changedNode) { Property.setProperty('/triggered', true); Property.removeListener(ident); }); "
      );
  BOOST_CHECK_EQUAL(propSys.getBoolValue("/triggered"), false);

  propSys.setIntValue("/testing", 2);
  sleep(1); // wait until listeners have run

  BOOST_CHECK_EQUAL(propSys.getBoolValue("/triggered"), true);
  BOOST_CHECK_EQUAL(propSys.getIntValue("/testing"), 2);

  propSys.setBoolValue("/triggered", false);
  sleep(1); // wait until listeners have run
  BOOST_CHECK_EQUAL(propSys.getBoolValue("/triggered"), false);

  propSys.setIntValue("/testing", 2);
  sleep(1); // wait until listeners have run
  BOOST_CHECK_EQUAL(propSys.getBoolValue("/triggered"), false);
  BOOST_CHECK_EQUAL(propSys.getIntValue("/testing"), 2);
}

BOOST_AUTO_TEST_CASE(testReentrancy) {
  PropertySystem propSys;
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new PropertyScriptExtension(propSys);
  env->addExtension(ext);

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  ctx->evaluate<void>("Property.setProperty('/testing', 1); Property.setProperty('/triggered', false); "
                      "var other_ident = Property.setListener('/triggered', function() { Property.setProperty('/itWorks', true); } ); "
                      "var listener_ident = Property.setListener('/testing', function(changedNode) { Property.setProperty('/triggered', true); }); "
      );

  propSys.setBoolValue("/testing", true);
  sleep(1); // wait until listeners have run
  BOOST_CHECK_EQUAL(propSys.getBoolValue("/itWorks"), true);

  ctx->evaluate<void>("Property.removeListener(other_ident); "
                      "Property.removeListener(listener_ident); "
      );

} // testReentrancy

BOOST_AUTO_TEST_CASE(testMultipleListeners) {
  PropertySystem propSys;
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new PropertyScriptExtension(propSys);
  env->addExtension(ext);

  boost::shared_ptr<ScriptContext> ctx(env->getContext());
  ctx->evaluate<void>(
      "var result = 0; Property.setProperty('/triggered', false); "
      "var l1 = Property.setListener('/triggered', function(changedNode) { Property.setProperty('/l1', true); result ++; } ); "
      "var l2 = Property.setListener('/triggered', function(changedNode) { Property.setProperty('/l2', true); result ++; } ); "
      "var l3 = Property.setListener('/triggered', function(changedNode) { Property.setProperty('/l3', true); result ++; }); "
  );

  propSys.setBoolValue("/triggered", true);
  sleep(1); // wait until listeners have run
  BOOST_CHECK_EQUAL(propSys.getBoolValue("/l1"), true);
  BOOST_CHECK_EQUAL(propSys.getBoolValue("/l2"), true);
  BOOST_CHECK_EQUAL(propSys.getBoolValue("/l3"), true);
  {
    JSContextThread thread(ctx);
    BOOST_CHECK_EQUAL(ctx->getRootObject().getProperty<int>("result"), 3);
  }

  ctx->evaluate<void>(
      "Property.removeListener(l1); "
      "Property.removeListener(l2); "
      "Property.removeListener(l3); "
  );

} // testMultipleListeners

BOOST_AUTO_TEST_CASE(testRemoveListener) {
  PropertySystem propSys;
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new PropertyScriptExtension(propSys);
  env->addExtension(ext);

  boost::shared_ptr<ScriptContext> ctx(env->getContext());
  ctx->evaluate<void>(
      "var result = 0; var listener; Property.setProperty('/triggered', false); "
      "function cb() { Property.removeListener(listener); Property.setProperty('/l1', true); result ++; }"
      "listener = Property.setListener('/triggered', cb); "
  );

  propSys.setBoolValue("/triggered", true);
  sleep(1); // wait until listeners have run
  BOOST_CHECK_EQUAL(propSys.getBoolValue("/l1"), true);
  {
    JSContextThread thread(ctx);
    BOOST_CHECK_EQUAL(ctx->getRootObject().getProperty<int>("result"), 1);
  }
} // testRemoveListener

class TestThreadingThread : public Thread {
public:
  TestThreadingThread(PropertyNodePtr _node)
  : Thread("TestThreadingThread"),
    m_pNode(_node)
  {
  }

  virtual void execute() {
    while(!m_Terminated) {
      m_pNode->setIntegerValue(2);
      sleepMS(100);
    }
    Logger::getInstance()->log("thread terminating");
  }

private:
  PropertyNodePtr m_pNode;
}; // TestThreadingThread

BOOST_AUTO_TEST_CASE(testThreading) {
  PropertySystem propSys;
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new PropertyScriptExtension(propSys);
  env->addExtension(ext);

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  ctx->evaluate<void>("Property.setProperty('/testing1', 1); Property.setProperty('/testing2', 1); "
                      "var l1 = Property.setListener('/testing1', function() { Property.setProperty('/itworks', true); }); "
                      "var l2 = Property.setListener('/testing2', function() { Property.setProperty('/itworks', true); }); "
      );

  PropertyNodePtr node1 = propSys.getProperty("/testing1");
  PropertyNodePtr node2 = propSys.getProperty("/testing2");

  // different nodes
  {
    Logger::getInstance()->log("different nodes");
    TestThreadingThread t1(node1);
    TestThreadingThread t2(node2);
    t1.run();
    t2.run();

    sleepSeconds(1);

    t1.terminate();
    t2.terminate();
    sleepMS(100);
  }

  // same node
  {
    Logger::getInstance()->log("same node");
    TestThreadingThread t1(node1);
    TestThreadingThread t2(node1);
    t1.run();
    t2.run();

    sleepSeconds(1);

    t1.terminate();
    t2.terminate();
    sleepMS(100);
  }

  ctx->evaluate<void>("Property.removeListener(l1); Property.removeListener(l2);");
} // testThreading

BOOST_AUTO_TEST_CASE(testPropertyFlags) {
  PropertySystem propSys;
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new PropertyScriptExtension(propSys);
  env->addExtension(ext);

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  PropertyNodePtr node1 = propSys.createProperty("/testing");

  ctx->evaluate<void>("Property.setFlag('/testing', 'ARCHIVE', true);");
  BOOST_CHECK_EQUAL(node1->hasFlag(PropertyNode::Archive), true);
  bool res = ctx->evaluate<bool>("Property.hasFlag('/testing', 'ARCHIVE')");
  BOOST_CHECK_EQUAL(res, true);

  ctx->evaluate<void>("Property.setFlag('/testing', 'READABLE', false);");
  BOOST_CHECK_EQUAL(node1->hasFlag(PropertyNode::Readable), false);
  res = ctx->evaluate<bool>("Property.hasFlag('/testing', 'READABLE')");
  BOOST_CHECK_EQUAL(res, false);

  ctx->evaluate<void>("Property.setFlag('/testing', 'WRITEABLE', false);");
  BOOST_CHECK_EQUAL(node1->hasFlag(PropertyNode::Writeable), false);
  res = ctx->evaluate<bool>("Property.hasFlag('/testing', 'WRITEABLE')");
  BOOST_CHECK_EQUAL(res, false);
} // testPropertyFlags

BOOST_AUTO_TEST_CASE(testPropertyObjExisting) {
  PropertySystem propSys;
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new PropertyScriptExtension(propSys);
  env->addExtension(ext);

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  PropertyNodePtr node1 = propSys.createProperty("/testing");

  ctx->evaluate<void>("var prop = new Property('/testing');\n"
                      "prop.setValue('test');\n"
  );

  BOOST_CHECK_EQUAL(node1->getStringValue(), "test");
} // testPropertyObjExisting

BOOST_AUTO_TEST_CASE(testPropertyObjNonExisting) {
  PropertySystem propSys;
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new PropertyScriptExtension(propSys);
  env->addExtension(ext);

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());

  ctx->evaluate<void>("var prop = new Property('/testing');\n"
                      "prop.setValue('test');\n"
  );

  PropertyNodePtr node1 = propSys.getProperty("/testing");
  BOOST_CHECK_EQUAL(node1->getStringValue(), "test");
} // testPropertyObjNonExisting

BOOST_AUTO_TEST_CASE(testPropertyObjNonExistingInvalid) {
  PropertySystem propSys;
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new PropertyScriptExtension(propSys);
  env->addExtension(ext);

  boost::shared_ptr<ScriptContext> ctx(env->getContext());
  ctx->evaluate<int>(
		  "var err = 0; \n"
		  "try { var prop = new Property('testing'); }\n"
          "catch (txt) { print(txt); err = 1; }\n");
  {
      JSContextThread thread(ctx);
      BOOST_CHECK_EQUAL(ctx->getRootObject().getProperty<int>("err"), 1);
  }
} // testPropertyObjNonExisting

BOOST_AUTO_TEST_CASE(testPropertyGetNodeExisting) {
  PropertySystem propSys;
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new PropertyScriptExtension(propSys);
  env->addExtension(ext);

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  PropertyNodePtr node1 = propSys.createProperty("/testing");

  ctx->evaluate<void>("var prop = Property.getNode('/testing');\n"
                      "prop.setValue('test');\n"
  );

  BOOST_CHECK_EQUAL(node1->getStringValue(), "test");
} // testPropertyGetNodeExisting

BOOST_AUTO_TEST_CASE(testPropertyGetChild) {
  PropertySystem propSys;
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new PropertyScriptExtension(propSys);
  env->addExtension(ext);

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  PropertyNodePtr node1 = propSys.createProperty("/testing/bla");

  ctx->evaluate<void>("var prop = Property.getNode('/testing');\n"
                      "prop.getChild('bla').setValue('test');\n"
  );

  BOOST_CHECK_EQUAL(node1->getStringValue(), "test");
} // testPropertyGetChild

BOOST_AUTO_TEST_CASE(testPropertyStoreReturnsFalse) {
  PropertySystem propSys;
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new PropertyScriptExtension(propSys);
  env->addExtension(ext);

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  bool res = ctx->evaluate<bool>("Property.store()");

  BOOST_CHECK_EQUAL(res, false);
} // testPropertyStoreReturnsFalse

BOOST_AUTO_TEST_CASE(testPropertyLoadReturnsFalse) {
  PropertySystem propSys;
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new PropertyScriptExtension(propSys);
  env->addExtension(ext);

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  bool res = ctx->evaluate<bool>("Property.load()");

  BOOST_CHECK_EQUAL(res, false);
} // testPropertyLoadReturnsFalse

BOOST_AUTO_TEST_CASE(testPropertyRemoveChild) {
  PropertySystem propSys;
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new PropertyScriptExtension(propSys);
  env->addExtension(ext);

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  ctx->evaluate<void>("Property.setProperty('/test/subnode', 'test');");

  BOOST_CHECK(propSys.getProperty("/test/subnode") != NULL);

  ctx->evaluate<void>("var node = Property.getNode('/test');\n"
                      "node.removeChild('subnode');");

  BOOST_CHECK(propSys.getProperty("/test/subnode") == NULL);
} // testPropertyRemoveChild

BOOST_AUTO_TEST_CASE(testPropertyRemoveChildNonexisting) {
  PropertySystem propSys;
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new PropertyScriptExtension(propSys);
  env->addExtension(ext);

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  ctx->evaluate<void>("Property.setProperty('/test', 'test');");

  ctx->evaluate<void>("var node = Property.getNode('/test');\n"
                      "node.removeChild('asdf');");
} // testPropertyRemoveChildNonexisting

BOOST_AUTO_TEST_CASE(testPropertyRemoveChildUndefined) {
  PropertySystem propSys;
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new PropertyScriptExtension(propSys);
  env->addExtension(ext);

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  ctx->evaluate<void>("Property.setProperty('/test', 'test');");

  ctx->evaluate<void>("var node = Property.getNode('/test');\n"
                      "var nonexistingNode = node.getChild('testing');\n"
                      "node.removeChild(nonexistingNode);");
} // testPropertyRemoveChildUndefined

BOOST_AUTO_TEST_CASE(testPropertyRemoveChildNull) {
  PropertySystem propSys;
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new PropertyScriptExtension(propSys);
  env->addExtension(ext);

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  ctx->evaluate<void>("Property.setProperty('/test', 'test');");

  ctx->evaluate<void>("var node = Property.getNode('/test');\n"
                      "node.removeChild(null);");
} // testPropertyRemoveChildNull

BOOST_AUTO_TEST_CASE(testPropertyGetParent) {
  PropertySystem propSys;
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new PropertyScriptExtension(propSys);
  env->addExtension(ext);

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  ctx->evaluate<void>("Property.setProperty('/test/subnode', 'test');");

  BOOST_CHECK(propSys.getProperty("/test/subnode") != NULL);

  ctx->evaluate<void>("var subnode = Property.getNode('/test/subnode');\n"
                      "subnode.getParent().removeChild(subnode);");

  BOOST_CHECK(propSys.getProperty("/test/subnode") == NULL);
} // testPropertyGetParent

BOOST_AUTO_TEST_SUITE_END()
