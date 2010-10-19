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

#define BOOST_TEST_NO_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <boost/filesystem.hpp>

#include "core/sim/dssim.h"
#include "core/model/modelmaintenance.h"
#include "core/model/apartment.h"
#include "core/model/set.h"
#include "core/sim/businterface/simbusinterface.h"

using namespace dss;

namespace fs = boost::filesystem;

BOOST_AUTO_TEST_SUITE(DSSimTest)

BOOST_AUTO_TEST_CASE(testConstruction) {
  DSSim sim(NULL);
  sim.initialize();
}


class DummyDevice : public DSIDInterface {
public:
  DummyDevice(const DSMeterSim& _simulator, dss_dsid_t _dsid, devid_t _shortAddress)
  : DSIDInterface(_simulator, _dsid, _shortAddress),
    m_Parameter(-2)
  { }

  virtual void callScene(const int _sceneNr) {
    functionCalled("callScene", _sceneNr);
  }

  virtual void saveScene(const int _sceneNr) {
    functionCalled("saveScene", _sceneNr);
  }

  virtual void undoScene() {
    functionCalled("undoScene");
  }

  virtual void enable() {
    functionCalled("enable");
  }

  virtual void disable() {
    functionCalled("disable");
  }

  virtual int getConsumption() {
    functionCalled("getConsumption");
    return 77;
  }

  virtual void setValue(const double _value, int _parameterNr = -1) {
    functionCalled("setValue");
  }

  virtual double getValue(int _parameterNr = -1) const {
    functionCalled("getValue", _parameterNr);
    return 0.0;
  }

  virtual uint16_t getFunctionID() {
    functionCalled("getFunctionID");
    return 1;
  }

  virtual void setConfigParameter(const std::string& _name, const std::string& _value) {
    functionCalled("setConfigParameter");
  }

  virtual std::string getConfigParameter(const std::string& _name) const {
    functionCalled("getConfigParameter");
    return "nothing";
  }

  const std::string& getCalledFunction() const {
    return m_CalledFunction;
  }

  int getParameter() const {
    return m_Parameter;
  }

private:
  void functionCalled(const std::string& _functionName, int _parameter = -1) const {
    m_CalledFunction = _functionName;
    m_Parameter = _parameter;
  }
private:
  mutable std::string m_CalledFunction;
  mutable int m_Parameter;
}; // DummyDevice

class DummyCreator : public DSIDCreator {
public:
  DummyCreator()
  : DSIDCreator("dummy")
  {}

  virtual ~DummyCreator() {};

  virtual DSIDInterface* createDSID(const dss_dsid_t _dsid, const devid_t _shortAddress, const DSMeterSim& _dsMeter) {
    return new DummyDevice(_dsMeter, _dsid, _shortAddress);
  }
};

class Fixture {
public:
  Fixture() {
    Apartment apt(NULL);
    m_pApartment.reset(new Apartment(NULL));
    m_pModelMaintenance.reset(new ModelMaintenance(NULL, 2));
    m_pModelMaintenance->setApartment(m_pApartment.get());
    m_pSimulation.reset(new DSSim(NULL));
    m_pSimulation->initialize();
    m_pSimulation->getDSIDFactory().registerCreator(new DummyCreator());

    std::string fileName = getTempDir() + "/sim.xml";
    std::ofstream ofs(fileName.c_str());
    ofs << "<?xml version=\"1.0\"?>\n"
          "<simulation version=\"1\">\n"
          "  <modulator busid=\"71\" dsid=\"13\">\n"
          "    <zone id=\"4\">\n"
          "      <device dsid=\"11\" busid=\"11\" type=\"dummy\"/>\n"
          "    </zone>\n"
          "  </modulator>\n"
          "</simulation>\n";
    ofs.close();

    m_pSimulation->loadFromFile(fileName);
    fs::remove(fileName);

    m_pBusInterface.reset(new SimBusInterface(m_pSimulation));
    m_pApartment->setBusInterface(m_pBusInterface.get());

    m_pModelMaintenance->setStructureQueryBusInterface(m_pBusInterface->getStructureQueryBusInterface());
    m_pModelMaintenance->initialize();
    m_pModelMaintenance->start();

    while(m_pModelMaintenance->isInitializing()) {
      sleepMS(2);
    }

    sleepMS(100);

    m_ValidDSID = DSSim::makeSimulatedDSID(dss_dsid_t(0, 0x11));
  }

  ~Fixture() {
    m_pModelMaintenance->shutdown();
    sleepMS(60);
  }
protected:
  boost::shared_ptr<Apartment> m_pApartment;
  boost::shared_ptr<ModelMaintenance> m_pModelMaintenance;
  boost::shared_ptr<DSSim> m_pSimulation;
  boost::shared_ptr<SimBusInterface> m_pBusInterface;
  dss_dsid_t m_ValidDSID;
};

BOOST_FIXTURE_TEST_CASE(testFixtureWorks, Fixture) {
  BOOST_CHECK_EQUAL(m_pApartment->getDevices().length(), 1);
  BOOST_CHECK_EQUAL(m_pApartment->getDevices().get(0).getDSID().toString(), m_ValidDSID.toString());
}

BOOST_FIXTURE_TEST_CASE(testCallSceneReachesDevice, Fixture) {
  DummyDevice* dev = dynamic_cast<DummyDevice*>(m_pSimulation->getSimulatedDevice(m_ValidDSID));
  BOOST_ASSERT(dev != NULL);

  const int kSceneNumber = 10;
  m_pApartment->getDevices().callScene(kSceneNumber);
  sleepMS(2);

  BOOST_CHECK_EQUAL(dev->getCalledFunction(), "callScene");
  BOOST_CHECK_EQUAL(dev->getParameter(), kSceneNumber);
}

BOOST_AUTO_TEST_SUITE_END()

