/*
 *  dss.cpp
 *  dSS
 *
 *  Created by Patrick Stählin on 4/2/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include "dss.h"
#include "logger.h"
#include "xmlwrapper.h"

#include "shttpd.h"

#include <cassert>
#include <string>
#include <iostream>

using namespace std;

namespace dss {

  //============================================= DSS
  
  DSS* DSS::m_Instance = NULL;
  
  DSS* DSS::GetInstance() {
    if(m_Instance == NULL) {
      m_Instance = new DSS();
    }
    assert(m_Instance != NULL);
    return m_Instance;
  } // GetInstance
  
  void DSS::Run() {
    Logger::GetInstance()->Log(L"DSS stating up....", lsInfo);
    LoadConfig();
    m_WebServer.Initialize(m_Config);
    m_WebServer.Run();
    while(true) {
      sleep(1000);
    }
  } // Run
  
  void DSS::LoadConfig() {
    Logger::GetInstance()->Log(L"Loading config", lsInfo);
    m_Config.ReadFromXML(L"/Users/packi/sources/dss/trunk/data/config.xml");
  } // LoadConfig

  //============================================= DSS
  
  WebServer::WebServer() :
    Thread(true, "WebServer")
  {
    m_SHttpdContext = shttpd_init();
  }
  
  WebServer::~WebServer() {
    shttpd_fini(m_SHttpdContext);
  }
  
  void WebServer::Initialize(Config& _config) {
    string ports = _config.GetOptionAs(L"webserverport", "8080");
    Logger::GetInstance()->Log(string("Webserver: Listening on port(s) ") + ports);
    shttpd_set_option(m_SHttpdContext, "ports", ports.c_str());

    string aliases = string("/=") + _config.GetOptionAs<string>(L"webserverroot", "/Users/packi/sources/dss/data/");
    Logger::GetInstance()->Log(string("Webserver: Configured aliases: ") + aliases);
    shttpd_set_option(m_SHttpdContext, "aliases", aliases.c_str());    

    shttpd_register_uri(m_SHttpdContext, "/config", &HTTPListOptions, NULL);    
  } // Initialize
  
  void WebServer::Execute() {
    Logger::GetInstance()->Log("Webserver started", lsInfo);
    while(!m_Terminated) {
      shttpd_poll(m_SHttpdContext, 1000);
    }
  }
  
  void WebServer::EmitHTTPHeader(int _code, struct shttpd_arg* _arg) {
    stringstream sstream;
    sstream << "HTTP/1.1 " << _code << " OK\r\n";
    sstream << "Content-Type: text/html; charset=utf-8\r\n\r\n";
    shttpd_printf(_arg, sstream.str().c_str());
  } // EmitHTTPHeader
  
  void WebServer::HTTPListOptions(struct shttpd_arg* _arg) {
    EmitHTTPHeader(200, _arg);
    shttpd_printf(_arg, "%s", "<html><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"><body>");
    shttpd_printf(_arg, "%s", "<h1>Configuration</h1>");
    
    wstringstream stream;
    stream << L"<ul>";
    
    const HashMapConstWStringWString& options = DSS::GetInstance()->GetConfig().GetOptions();
    for(HashMapConstWStringWString::const_iterator iOption = options.begin(); iOption != options.end(); ++iOption) {
      stream << L"<li>" << iOption->first << L" = " << iOption->second << L"</li>";
    }
    stream << L"</ul></body></html>";
    shttpd_printf(_arg, ToUTF8(stream.str()).c_str());
    
    _arg->flags |= SHTTPD_END_OF_OUTPUT;
  } // HTTPListOptions
  
  //============================================= Config
  
  bool Config::HasOption(const wstring& _name) const {
    return m_OptionsByName.find(_name) != m_OptionsByName.end();
  }
  
  void Config::AssertHasOption(const wstring& _name) const {
    if(!HasOption(_name)) {
      throw new NoSuchOptionException(_name);
    }
  } // AssertHasOption
  
  const int ConfigVersion = 1;

  void Config::ReadFromXML(const wstring& _fileName) {
    XMLDocumentFileReader reader(_fileName);
    XMLNode rootNode = reader.GetDocument().GetRootNode();
    if(rootNode.GetName() == L"config") {
      if(StrToInt(rootNode.GetAttributes()[L"version"]) == ConfigVersion) {
        Logger::GetInstance()->Log(L"Right, version, going to read items");
        
        XMLNodeList items = rootNode.GetChildren();
        for(XMLNodeList::iterator it = items.begin(); it != items.end(); ++it) {
          if(it->GetName() == L"item") {
            try {
              XMLNode& nameNode = it->GetChildByName(L"name");
              if(nameNode.GetChildren().size() == 0) {
                continue;
              }
              nameNode = (nameNode.GetChildren())[0];
              
              XMLNode& valueNode = it->GetChildByName(L"value");
              if(valueNode.GetChildren().size() == 0) {
                continue;
              }
              valueNode = (valueNode.GetChildren())[0];
              m_OptionsByName[nameNode.GetContent()] = valueNode.GetContent();
            } catch(XMLException& _e) {
              Logger::GetInstance()->Log(string("Error loading XML-File: ") + _e.what(), lsError);
            }
          }
        }
      }
    }
  } // ReadFromXML
  
  template <>
  int Config::GetOptionAs(const wstring& _name) const {
    AssertHasOption(_name);

    return wcstol(m_OptionsByName[_name].c_str(), NULL, 10);
  } // GetOptionAs<int>
  
  template <>
  wstring Config::GetOptionAs(const wstring& _name) const {
    AssertHasOption(_name);
    
    return m_OptionsByName[_name];
  } // GetOptionAs<wstring>
                                          
  template <>
  string Config::GetOptionAs(const wstring& _name) const {
    AssertHasOption(_name);
    
    wstring wresult = m_OptionsByName[_name];
    return ToUTF8(wresult.c_str(), wresult.size());                  
  } // GetOptionAs<string>
  
  template <>
  const char* Config::GetOptionAs(const wstring& _name) const {
    return GetOptionAs<string>(_name).c_str();
  }
  
}