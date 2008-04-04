#include <iostream>

#include "base.h"
#include "dss.h"
#include "core/xmlwrapper.h"

#include <libxml/tree.h>
#include <libxml/encoding.h>

using namespace std;
using namespace dss;

void testMBCS();
void testXMLReader();
void testConfig();


void cppTest();

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
  
  testConfig();
  //cppTest();
  //testXMLReader();
 // testMBCS();
  // start DSS
  dss::DSS::GetInstance()->Run();
  return 0;
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
  conf.ReadFromXML(wstring(L"/Users/packi/sources/dss/data/testconfig.xml"));
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
