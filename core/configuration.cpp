#include "configuration.h"
#include "xmlwrapper.h"
#include "logger.h"

namespace dss {
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
              (nameNode.GetChildren()).at(0);
              nameNode = (nameNode.GetChildren()).at(0);
              
              XMLNode& valueNode = it->GetChildByName("value");
              if(valueNode.GetChildren().size() == 0) {
                continue;
              }
              valueNode = (valueNode.GetChildren()).at(0);
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