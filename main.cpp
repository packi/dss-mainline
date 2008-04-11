/*
 *  Copyright:
 *  (c) 2008 by
 *  futureLAB AG
 *  Schwalmenackerstrasse 4
 *  CH-8400 Winterthur / Schweiz
 *  Alle Rechte vorbehalten.
 *  Jede Art der Vervielfaeltigung, Verbreitung,
 *  Auswertung oder Veraenderung - auch auszugsweise -
 *  ist ohne vorgaengige schriftliche Genehmigung durch
 *  die futureLAB AG untersagt.
 *
 * Last change $Date$
 * by $Author$
*/



#include <iostream>

#include "base.h"
#include "dss.h"
#include "core/xmlwrapper.h"
#include "core/jshandler.h"

#include <libxml/tree.h>
#include <libxml/encoding.h>

using namespace std;
using namespace dss;

void testMBCS();
void testXMLReader();
void testConfig();
int testJS();

void testJS2() {
  ScriptEnvironment env;
  env.Initialize();
  ScriptContext* ctx = env.GetContext();
  ctx->LoadFromMemory("x = 10; print(x); x = x * x;");
  double result = ctx->Evaluate<double>();
  assert(result == 100.0);  
  delete ctx;
  
  ctx = env.GetContext();
  ctx->LoadFromFile("/Users/packi/sources/dss/trunk/data/test.js");
  result = ctx->Evaluate<double>();
  assert(result == 100.0);
  delete ctx;
  
  ctx = env.GetContext();
  ctx->LoadFromMemory("x = 'bla'; x = x + 'bla';");
  string sres = ctx->Evaluate<string>();
  assert(sres == "blabla");
  delete ctx;
}


void cppTest();
int runTests();

int main (int argc, char * const argv[]) {
  
  if (!setlocale(LC_CTYPE, "")) {
    fprintf(stderr, "Can't set the specified locale! "
            "Check LANG, LC_CTYPE, LC_ALL.\n");
    return 1;
  }
  // disable broken pipe signal
#ifndef _WIN32
	signal(SIGPIPE, SIG_IGN);
#endif  
  xmlInitParser();
  
  testJS2();

  //runTests();
  //testConfig();
  //cppTest();
  //testXMLReader();
 // testMBCS();
  // start DSS
  dss::DSS::GetInstance()->Run();
  return 0;
}

#include <js/jsapi.h>

JSBool
global_newresolve(JSContext *cx, JSObject *obj, jsval id,
                  uintN flags, JSObject **objp)
{
  /* taken from js.c - I don't understand the flags yet */
  if ((flags & JSRESOLVE_ASSIGNING) == 0) {
    JSBool resolved;
    if (!JS_ResolveStandardClass(cx, obj, id, &resolved))
      return JS_FALSE;
    if (resolved)
      *objp = obj;
  }
  return JS_TRUE;
}


static void js_error_handler(JSContext *ctx, const char *msg, JSErrorReport *er){
  char *pointer=NULL;
  char *line=NULL;
  int len;
  
  if (er->linebuf != NULL){
    len = er->tokenptr - er->linebuf + 1;
    pointer = (char*)malloc(len);
    memset(pointer, '-', len);
    pointer[len-1]='\0';
    pointer[len-2]='^';
    
    len = strlen(er->linebuf)+1;
    line = (char*)malloc(len);
    strncpy(line, er->linebuf, len);
    line[len-1] = '\0';
  }
  else {
    len=0;
    pointer = (char*)malloc(1);
    line = (char*)malloc(1);
    pointer[0]='\0';
    line[0] = '\0';
  }
  
  while (len > 0 && (line[len-1] == '\r' || line[len-1] == '\n')){ line[len-1]='\0'; len--; }
  
  printf("JS Error: %s\nFile: %s:%u\n", msg, er->filename, er->lineno);
  if (line[0]){
    printf("%s\n%s\n", line, pointer);
  }
  
  free(pointer);
  free(line);
}

static JSClass global_class = {
  "global", JSCLASS_NEW_RESOLVE, /* use the new resolve hook */
  JS_PropertyStub,  JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
  /* todo: explain */
  JS_EnumerateStandardClasses,
  JS_ResolveStub,
  JS_ConvertStub,  JS_FinalizeStub, JSCLASS_NO_OPTIONAL_MEMBERS
};

JSBool global_print(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval){
  if (argc < 1){
    /* No arguments passed in, so do nothing. */
    /* We still want to return JS_TRUE though, other wise an exception will be thrown by the engine. */
    *rval = INT_TO_JSVAL(0); /* Send back a return value of 0. */
  } else {
    int i;
    size_t amountWritten=0;
    for (i=0; i<argc; i++){
      JSString *val = JS_ValueToString(cx, argv[i]); /* Convert the value to a javascript string. */
      char *str = JS_GetStringBytes(val); /* Then convert it to a C-style string. */
      size_t length = JS_GetStringLength(val); /* Get the length of the string, # of chars. */
      amountWritten = fwrite(str, sizeof(*str), length, stdout); /* write the string to stdout. */
    }
    *rval = INT_TO_JSVAL(amountWritten); /* Set the return value to be the number of bytes/chars written */
  }
  fwrite("\n", 1, 1, stdout);
  return JS_TRUE;
}

JSFunctionSpec global_methods[] = {
  {"print", global_print, 1, 0, 0},
  {NULL},
};


int testJS() { 
  JSRuntime *rt;
  JSContext *cx;
  
  JSObject  *global;

  /* Create a JS runtime. */
  rt = JS_NewRuntime(8L * 1024L * 1024L);
  if (rt == NULL)
    return 1;
  
  /* Create a context. */
  cx = JS_NewContext(rt, 8192);
  if (cx == NULL)
    return 1;
  JS_SetOptions(cx, JSOPTION_VAROBJFIX);
  JS_SetErrorReporter(cx, js_error_handler);
  
  /* Create the global object. */
  global = JS_NewObject(cx, &global_class, NULL, NULL);
  if (global == NULL)
    return 1;
  
  JS_DefineFunctions(cx, global, global_methods);
  
  /* Populate the global object with the standard globals,
   like Object and Array. */
  if (!JS_InitStandardClasses(cx, global))
    return 1;
  
  
  
  /*
   * The return value comes back here -- if it could be a GC thing, you must
   * add it to the GC's "root set" with JS_AddRoot(cx, &thing) where thing
   * is a JSString *, JSObject *, or jsdouble *, and remove the root before
   * rval goes out of scope, or when rval is no longer needed.
   */
  jsval rval;
  JSBool ok;
  
  /*
   * Some example source in a C string.  Larger, non-null-terminated buffers
   * can be used, if you pass the buffer length to JS_EvaluateScript.
   */
  char *source = /*"var bla = {};\
  bla.xy = 1;\
  return true;";*/ 
  "var x=10.0;print(x);x = Math.sqrt(x*x*x*x);print(x);x;";
  
  ok = JS_EvaluateScript(cx, global, source, strlen(source),
                         "test", 1, &rval);
  
  if (ok) {
    /* Should get a number back from the example source. */
    jsdouble d;
    
    ok = JS_ValueToNumber(cx, rval, &d);
    cout << d << endl;
    
  }
  return 0;
}


#include <cppunit/TestRunner.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/BriefTestProgressListener.h>
#include <cppunit/extensions/TestFactoryRegistry.h>

class Test : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(Test);
  CPPUNIT_TEST(testHelloWorld);
  CPPUNIT_TEST_SUITE_END();
  
public:
  void setUp(void) {}
  void tearDown(void) {} 
  
protected:
  void testHelloWorld(void) { 
    CPPUNIT_ASSERT_EQUAL( 1, 2 );
    std::cout << "Hello, world!" << std::endl; 
  }
};

CPPUNIT_TEST_SUITE_REGISTRATION(Test);

int runTests() {
  //--- Create the event manager and test controller
  CPPUNIT_NS::TestResult controller;
  
  //--- Add a listener that colllects test result
  CPPUNIT_NS::TestResultCollector result;
  controller.addListener( &result );        
  
  //--- Add a listener that print dots as test run.
  CPPUNIT_NS::BriefTestProgressListener progress;
  controller.addListener( &progress );      
  
  //--- Add the top suite to the test runner
  CPPUNIT_NS::TestRunner runner;
  runner.addTest( CPPUNIT_NS::TestFactoryRegistry::getRegistry().makeTest() );
  runner.run( controller );
  
  return result.wasSuccessful() ? 0 : 1;
}

void testMBCS() {
  xmlDoc* doc = NULL;
  xmlNode *rootElem = NULL;
  
  doc = xmlParseFile("/Users/packi/sources/dss/data/test.xml");
  if(doc == NULL) {
    wcout << L"Could not open test-file (data/test.xml)" << endl;
  }
  
  rootElem = xmlDocGetRootElement( doc );

  cout << "UTF-8 " << rootElem->children->content << endl; 
/*
  wchar_t bla[100];
  size_t len = mbsrtowcs(bla, (const char**)&rootElem->children->content, 100, NULL);
 // wstring blaw(bla, len);
*/
  
  const char* utfStr = (const char*)rootElem->children->content;
  
 wcout << "WCS   " << FromUTF8(utfStr, strlen(utfStr)) << endl;
  
  //cout << rootElem->children->name << endl;
  //cout << (rootElem->children->content) << endl;
  cout << "\u221E" << endl;
} // testMBCS

void testXMLReader() {
  XMLDocumentFileReader reader(L"/Users/packi/sources/dss/data/test.xml");
  XMLDocument& doc = reader.GetDocument();
  XMLNode& rootNode = doc.GetRootNode();
  wcout << rootNode.GetName() << endl;
  wcout << rootNode.GetChildren().size() << endl;
  vector<XMLNode> children = rootNode.GetChildren();
  wcout << children[0].GetName() << endl;
  wcout << children[0].GetContent() << endl;
  wcout << children[0].GetChildren().size() << endl;
//  assert(L"02日22時更新" == children[0].GetContent());
} // testXMLReader

void testConfig() {
  Config conf;
  conf.ReadFromXML(wstring(L"/Users/packi/sources/dss/trunk/data/testconfig.xml"));
  int bla = conf.GetOptionAs<int>(L"serverport");
  assert(bla == 8080);
  
  wstring serverRoot = conf.GetOptionAs<wstring>(L"serverroot");
  wcout << serverRoot << endl;
  
  assert(conf.GetOptionAs(L"bla", 1234) == 1234);
  
  conf.SetOptionAs(L"bla", 1234);
  
  assert(conf.HasOption(L"bla"));
  assert(conf.GetOptionAs<int>(L"bla") == 1234);
  assert(conf.GetOptionAs(L"bla", 4321) == 1234);
}

class Aa {
private:
  mutable int m_A;
  
  int Release() const {
    int result = m_A;
    m_A = 0;
    return result;
  }
public:
//  Aa() : m_A(0) {}
  explicit Aa(int _a = 0) : m_A( _a ) { cout << "default constructor with " << _a << endl; }
 
  /* 
  Aa(const Aa& _other) 
  : m_A(_other.Release())
  {
    cout << "copy (const) constuctor" << endl;
  }
   */

  Aa(Aa& _other)
  : m_A(_other.Release())
  {
    cout << "copy constuctor" << endl;
  }
  int GetA() { return m_A; }
  
  Aa& operator=(Aa& _other) {
    m_A = _other.Release();
    return *this;
  }
};

void cppTest() {
  cout << "creating with default" << endl;
  Aa aOrig;
  cout << "creating with 7" << endl;
  Aa aNew(7);
  cout << "assigning" << endl;
  aOrig = aNew;
  cout << aNew.GetA() << endl;
  cout << aOrig.GetA() << endl;
}
