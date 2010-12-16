/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland

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

#include "dssim.h"

#include "dsidsim.h"
#include "dsmetersim.h"

#include <string>
#include <stdexcept>

#include <boost/filesystem.hpp>

#include "core/model/modelconst.h"
#include "core/base.h"
#include "core/logger.h"
#include "core/dss.h"
#include "core/businterface.h"
#include "core/foreach.h"
#include "core/propertysystem.h"
#include "dsid_js.h"

#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/Node.h>
#include <Poco/DOM/Attr.h>
#include <Poco/DOM/Text.h>
#include <Poco/DOM/AutoPtr.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/SAX/InputSource.h>
#include <Poco/SAX/SAXException.h>

using Poco::XML::Document;
using Poco::XML::Element;
using Poco::XML::Attr;
using Poco::XML::Text;
using Poco::XML::ProcessingInstruction;
using Poco::XML::AutoPtr;
using Poco::XML::DOMParser;
using Poco::XML::InputSource;
using Poco::XML::Node;

namespace dss {

  //================================================== DSIDSimCreator

  class DSIDSimCreator : public DSIDCreator {
  public:
    DSIDSimCreator()
    : DSIDCreator("standard.simple")
    {}

    virtual ~DSIDSimCreator() {};

    virtual DSIDInterface* createDSID(const dss_dsid_t _dsid, const devid_t _shortAddress, const DSMeterSim& _dsMeter) {
      return new DSIDSim(_dsMeter, _dsid, _shortAddress);
    }
  };


  //================================================== DSSim

  DSSim::DSSim(DSS* _pDSS)
  : Subsystem(_pDSS, "DSSim")
  {}

  void DSSim::initialize() {
    Subsystem::initialize();
    m_DSIDFactory.registerCreator(new DSIDSimCreator());

    if(DSS::hasInstance()) {
      PropertyNodePtr pNode = getDSS().getPropertySystem().getProperty(getConfigPropertyBasePath() + "js-devices");
      if(pNode != NULL) {
        for(int iNode = 0; iNode < pNode->getChildCount(); iNode++) {
          createJSPluginFrom(pNode->getChild(iNode));
        }
      }

      getDSS().getPropertySystem().setStringValue(getConfigPropertyBasePath() + "configfile", getDSS().getConfigDirectory() + "sim.xml", true, false);

      boost::filesystem::path filename(getDSS().getPropertySystem().getStringValue(getConfigPropertyBasePath() + "configfile"));
     
      loadFromConfig();
    }

    m_Initialized = true;
  } // initialize

  void DSSim::createJSPluginFrom(PropertyNodePtr _node) {
    PropertyNodePtr simIDNode = _node->getPropertyByName("id");
    if(simIDNode == NULL) {
      log("DSSim::createJSPluginFrom: Missing property id", lsError);
      return;
    }
    std::string simID = simIDNode->getAsString();
    log("DSSim::createJSPluginFrom: Found device with id: " + simID, lsInfo);
    std::vector<std::string> scripts;
    for(int iNode = 0; iNode < _node->getChildCount(); iNode++) {
      PropertyNodePtr childNode = _node->getChild(iNode);
      if(childNode->getName() == "script-file") {
        std::string scriptFile = childNode->getAsString();
        log("DSSim::createJSPluginFrom:   adding script-file: " + scriptFile, lsInfo);
        if(!boost::filesystem::exists(scriptFile)) {
          log("DSSim::createJSPluginFrom: cannot find script file '" + scriptFile + "', skipping device " + simID, lsError);
          return;
        }
        scripts.push_back(scriptFile);
      }
    }
    if(!scripts.empty()) {
      m_DSIDFactory.registerCreator(new DSIDJSCreator(scripts, simID, *this));
    } else {
      log("DSSim::createJSPluginFrom: Missing property script-file", lsError);
    }
  } // createJSPluginFrom

  void DSSim::loadFromConfig() {
    loadFromFile(getDSS().getPropertySystem().getStringValue(getConfigPropertyBasePath() + "configfile"));
  }

  void DSSim::loadFromFile(const std::string& _fileName) {
    const int theConfigFileVersion = 1;
    std::ifstream inFile(_fileName.c_str());

    // file does not exist, this is allowed, ignore silently
    if (!inFile.is_open()) {
      return;
    }

    try {
      InputSource input(inFile);
      DOMParser parser;
      AutoPtr<Document> pDoc = parser.parse(&input);
      Element* rootNode = pDoc->documentElement();

      if(rootNode->localName() == "simulation") {
        if(rootNode->hasAttribute("version") && (strToInt(rootNode->getAttribute("version")) == theConfigFileVersion)) {
          Node* curNode = rootNode->firstChild();
          while(curNode != NULL) {
            if(curNode->localName() == "modulator") {
              boost::shared_ptr<DSMeterSim> dsMeter;

              dsMeter.reset(new DSMeterSim(this));
              dsMeter->initializeFromNode(curNode);
              m_DSMeters.push_back(dsMeter);
            }
            curNode = curNode->nextSibling();
          }
        }
      } else {
        throw std::runtime_error(_fileName +
                " must have a root-node named 'simulation'");
      }
    } catch(Poco::XML::SAXParseException& e) {
      throw std::runtime_error("Error parsing file: " + _fileName + ": " +
                               e.message());
    }
  } // loadFromConfig

  bool DSSim::isReady() {
    return m_Initialized;
  } // ready

//  void DSSim::distributeFrame(boost::shared_ptr<DS485CommandFrame> _frame) {
//    DS485FrameProvider::distributeFrame(_frame);
//  } // distributeFrame

  DSIDInterface* DSSim::getSimulatedDevice(const dss_dsid_t& _dsid) {
    DSIDInterface* result = NULL;
    foreach(boost::shared_ptr<DSMeterSim> dsMeter, m_DSMeters) {
      result = dsMeter->getSimulatedDevice(_dsid);
      if(result != NULL) {
        break;
      }
    }
    return result;
  } // getSimulatedDevice

  boost::shared_ptr<DSMeterSim> DSSim::getDSMeter(const dss_dsid_t& _dsid) {
    boost::shared_ptr<DSMeterSim> result;
    foreach(boost::shared_ptr<DSMeterSim> dsMeter, m_DSMeters) {
      if(dsMeter->getDSID() == _dsid) {
        result = dsMeter;
        break;
      }
    }
    return result;
  } // getDSMeter

  dss_dsid_t DSSim::makeSimulatedDSID(const dss_dsid_t& _dsid) {
    dss_dsid_t result = _dsid;
    result.upper = (result.upper & 0x000000000000000Fll) | DSIDHeader;
    result.lower = (result.lower & 0x002FFFFF) | SimulationPrefix;
    return result;
  } // makeSimulatedDSID


  //================================================== DSIDCreator

  DSIDCreator::DSIDCreator(const std::string& _identifier)
  : m_Identifier(_identifier)
  {} // ctor


  //================================================== DSIDFactory

  DSIDInterface* DSIDFactory::createDSID(const std::string& _identifier, const dss_dsid_t _dsid, const devid_t _shortAddress, const DSMeterSim& _dsMeter) {
    boost::ptr_vector<DSIDCreator>::iterator
    iCreator = m_RegisteredCreators.begin(),
    e = m_RegisteredCreators.end();
    while(iCreator != e) {
      if(iCreator->getIdentifier() == _identifier) {
        return iCreator->createDSID(_dsid, _shortAddress, _dsMeter);
      }
      ++iCreator;
    }
    Logger::getInstance()->log(std::string("Could not find creator for DSID type '") + _identifier + "'");
    throw std::runtime_error(std::string("Could not find creator for DSID type '") + _identifier + "'");
  } // createDSID

  void DSIDFactory::registerCreator(DSIDCreator* _creator) {
    m_RegisteredCreators.push_back(_creator);
  } // registerCreator

}
