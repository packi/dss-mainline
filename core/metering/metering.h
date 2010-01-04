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

#ifndef METERING_H_
#define METERING_H_

#include "core/thread.h"
#include "core/subsystem.h"
#include "core/datetools.h"

#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>

namespace dss {

  class MeteringConfig;
  class MeteringConfigChain;

  class Metering : public Subsystem,
                   private Thread {
  private:
    int m_MeterEnergyCheckIntervalSeconds;
    int m_MeterConsumptionCheckIntervalSeconds;
    std::string m_MeteringStorageLocation;
    std::vector<boost::shared_ptr<MeteringConfigChain> > m_Config;
  private:
    void checkModulators(boost::shared_ptr<MeteringConfigChain> _config);

    virtual void execute();
  protected:
    virtual void doStart();
  public:
    Metering(DSS* _pDSS);
    virtual ~Metering() {};
    
    const std::vector<boost::shared_ptr<MeteringConfigChain> > getConfig() const { return m_Config; }
    const std::string& getStorageLocation() const { return m_MeteringStorageLocation; }
  }; // Metering

  class MeteringConfig {
  private:
    std::string m_FilenameSuffix;
    int m_Resolution;
    int m_NumberOfValues;
  public:
    MeteringConfig(const std::string& _filenameSuffix, int _resolution, int _numberOfValues)
    : m_FilenameSuffix(_filenameSuffix),
      m_Resolution(_resolution),
      m_NumberOfValues(_numberOfValues)
    { } // ctor

    const std::string& getFilenameSuffix() const { return m_FilenameSuffix; }
    const int getResolution() const { return m_Resolution; }
    const int getNumberOfValues() const { return m_NumberOfValues; }
  }; // MeteringConfig

  class MeteringConfigChain {
  private:
    bool m_IsEnergy;
    int m_CheckIntervalSeconds;
    DateTime m_LastRun;
    std::string m_Unit;
    std::string m_Comment;
    std::vector<boost::shared_ptr<MeteringConfig> > m_Chain;
  public:
    MeteringConfigChain(bool _isEnergy, int _checkIntervalSeconds, const std::string& _unit)
    : m_IsEnergy(_isEnergy), m_CheckIntervalSeconds(_checkIntervalSeconds), m_Unit(_unit)
    { }

    void addConfig(boost::shared_ptr<MeteringConfig> _config);
    bool needsRun() const;
    void running();

    const int size() const { return m_Chain.size(); }

    const std::string& getFilenameSuffix(int _index) const { return m_Chain.at(_index)->getFilenameSuffix(); }
    const int getResolution(int _index) const { return m_Chain.at(_index)->getResolution(); }
    const int getNumberOfValues(int _index) const { return m_Chain.at(_index)->getNumberOfValues(); }

    bool isEnergy() const { return m_IsEnergy; }
    bool isConsumption() const { return !m_IsEnergy; }
    int getCheckIntervalSeconds() const { return m_CheckIntervalSeconds; }
    const std::string& getUnit() const { return m_Unit; }
    const std::string& getComment() const { return m_Comment; }
    void setComment(const std::string& _value) { m_Comment = _value; }
  }; // MeteringConfigChain

} // namespace dss

#endif /* METERING_H_ */
