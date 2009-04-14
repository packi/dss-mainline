/*
 * metering.h
 *
 *  Created on: Jan 29, 2009
 *      Author: patrick
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
    void CheckModulators(boost::shared_ptr<MeteringConfigChain> _config);

    virtual void Execute();
  protected:
    virtual void DoStart();
  public:
    Metering(DSS* _pDSS);
    virtual ~Metering() {};

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

    const std::string& GetFilenameSuffix() const { return m_FilenameSuffix; }
    const int GetResolution() const { return m_Resolution; }
    const int GetNumberOfValues() const { return m_NumberOfValues; }
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

    void AddConfig(boost::shared_ptr<MeteringConfig> _config);
    bool NeedsRun() const;
    void Running();

    const int Size() const { return m_Chain.size(); }

    const std::string& GetFilenameSuffix(int _index) const { return m_Chain.at(_index)->GetFilenameSuffix(); }
    const int GetResolution(int _index) const { return m_Chain.at(_index)->GetResolution(); }
    const int GetNumberOfValues(int _index) const { return m_Chain.at(_index)->GetNumberOfValues(); }

    bool IsEnergy() const { return m_IsEnergy; }
    bool IsConsumption() const { return !m_IsEnergy; }
    int GetCheckIntervalSeconds() const { return m_CheckIntervalSeconds; }
    const std::string& GetUnit() const { return m_Unit; }
    const std::string& GetComment() const { return m_Comment; }
    void SetComment(const std::string& _value) { m_Comment = _value; }
  }; // MeteringConfigChain

} // namespace dss

#endif /* METERING_H_ */
