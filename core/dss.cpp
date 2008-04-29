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
    Logger::GetInstance()->Log("DSS stating up....", lsInfo);
    LoadConfig();
    m_WebServer.Initialize(m_Config);
    m_WebServer.Run();
    
    m_ModulatorSim.Initialize();
    m_Apartment.Run();
    
    while(true) {
      sleep(1000);
    }
  } // Run
  
  void DSS::LoadConfig() {
    Logger::GetInstance()->Log("Loading config", lsInfo);
    m_Config.ReadFromXML("/Users/packi/sources/dss/trunk/data/config.xml");
  } // LoadConfig

  //============================================= DSS
  
  WebServer::WebServer() 
  : Thread(true, "WebServer")
  {
    m_SHttpdContext = shttpd_init();
  } // ctor
  
  WebServer::~WebServer() {
    shttpd_fini(m_SHttpdContext);
  } // dtor
  
  void WebServer::Initialize(Config& _config) {
    string ports = _config.GetOptionAs("webserverport", "8080");
    Logger::GetInstance()->Log(string("Webserver: Listening on port(s) ") + ports);
    shttpd_set_option(m_SHttpdContext, "ports", ports.c_str());

    string aliases = string("/=") + _config.GetOptionAs<string>("webserverroot", "/Users/packi/sources/dss/data/");
    Logger::GetInstance()->Log(string("Webserver: Configured aliases: ") + aliases);
    shttpd_set_option(m_SHttpdContext, "aliases", aliases.c_str());    

    shttpd_register_uri(m_SHttpdContext, "/config", &HTTPListOptions, NULL);    
    shttpd_register_uri(m_SHttpdContext, "/json/*", &JSONHandler, NULL);
  } // Initialize
  
  void WebServer::Execute() {
    Logger::GetInstance()->Log("Webserver started", lsInfo);
    while(!m_Terminated) {
      shttpd_poll(m_SHttpdContext, 1000);
    }
  } // Execute
  
  void WebServer::EmitHTTPHeader(int _code, struct shttpd_arg* _arg, string _contentType) {
    stringstream sstream;
    sstream << "HTTP/1.1 " << _code << " OK\r\n";
    sstream << "Content-Type: " << _contentType << "; charset=utf-8\r\n\r\n";
    shttpd_printf(_arg, sstream.str().c_str());
  } // EmitHTTPHeader
  
  void WebServer::JSONHandler(struct shttpd_arg* _arg) {
    string urlid = "/json/";
    string uri = shttpd_get_env(_arg, "REQUEST_URI");
    const char* paramsC = shttpd_get_env(_arg, "QUERY_STRING");
    string params;
    if(paramsC != NULL) {
      params = paramsC;
    }
        
    string method = uri.substr(uri.find(urlid) + urlid.size());
    
    if(method == "getdevices") {
      EmitHTTPHeader(200, _arg, "application/json");
      Set devices = DSS::GetInstance()->GetApartment().GetDevices();
      
      stringstream sstream;
      sstream << "{\"devices\":[";
      for(int iDevice = 0; iDevice < devices.Length(); iDevice++) {
        DeviceReference& d = devices.Get(iDevice);
        sstream << "{ \"id\": " << d.GetID() << ", \"name\": \"" << d.GetDevice().GetName() << "\" }";        
      }
      sstream << "]}";
        
      shttpd_printf(_arg, sstream.str().c_str());
    } else {
      shttpd_printf(_arg, "hello %s, method %s, params %s", uri.c_str(), method.c_str(), params.c_str());
    }
    _arg->flags |= SHTTPD_END_OF_OUTPUT;
  } // JSONHandler
  
  void WebServer::HTTPListOptions(struct shttpd_arg* _arg) {
    EmitHTTPHeader(200, _arg);
    shttpd_printf(_arg, "%s", "<html><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"><body>");
    shttpd_printf(_arg, "%s", "<h1>Configuration</h1>");
    
    stringstream stream;
    stream << "<ul>";
    
    const HashMapConstStringString& options = DSS::GetInstance()->GetConfig().GetOptions();
    for(HashMapConstStringString::const_iterator iOption = options.begin(); iOption != options.end(); ++iOption) {
      stream << "<li>" << iOption->first << " = " << iOption->second << "</li>";
    }
    stream << "</ul></body></html>";
    shttpd_printf(_arg, stream.str().c_str());
    
    _arg->flags |= SHTTPD_END_OF_OUTPUT;
  } // HTTPListOptions
  
  //============================================= Config
  
  bool Config::HasOption(const string& _name) const {
    return m_OptionsByName.find(_name) != m_OptionsByName.end();
  } // HasOption
  
  void Config::AssertHasOption(const string& _name) const {
    if(!HasOption(_name)) {
      throw new NoSuchOptionException(_name);
    }
  } // AssertHasOption
  
  const int ConfigVersion = 1;

  void Config::ReadFromXML(const string& _fileName) {
    XMLDocumentFileReader reader(_fileName);
    XMLNode rootNode = reader.GetDocument().GetRootNode();
    if(rootNode.GetName() == "config") {
      if(StrToInt(rootNode.GetAttributes()["version"]) == ConfigVersion) {
        Logger::GetInstance()->Log("Right, version, going to read items");
        
        XMLNodeList items = rootNode.GetChildren();
        for(XMLNodeList::iterator it = items.begin(); it != items.end(); ++it) {
          if(it->GetName() == "item") {
            try {
              XMLNode& nameNode = it->GetChildByName("name");
              if(nameNode.GetChildren().size() == 0) {
                continue;
              }
              nameNode = (nameNode.GetChildren())[0];
              
              XMLNode& valueNode = it->GetChildByName("value");
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
  int Config::GetOptionAs(const string& _name) const {
    AssertHasOption(_name);

    return StrToInt(m_OptionsByName[_name]);
  } // GetOptionAs<int>
  
  template <>
  string Config::GetOptionAs(const string& _name) const {
    AssertHasOption(_name);
    
    return m_OptionsByName[_name];
  } // GetOptionAs<string>
                                          
  template <>
  const char* Config::GetOptionAs(const string& _name) const {
    return GetOptionAs<string>(_name).c_str();
  } // GetOptionAs<const char*>
  
}