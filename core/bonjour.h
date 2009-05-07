#ifndef NEUROBONJOUR_H
#define NEUROBONJOUR_H

#include "thread.h"

#include <vector>

#include <boost/ptr_container/ptr_vector.hpp>

#include <dns_sd.h>

namespace dss {


  class LookupRecord {
    private:
      std::string m_HostName;
      std::string m_FullName;
      int m_Port;
      int m_IP;
    public:
      LookupRecord()
      : m_HostName(""),
        m_FullName(""),
        m_Port(0),
        m_IP(0)
      {}

      void SetHostName(const std::string& _value) { m_HostName = _value; };
      const std::string& GetHostName() const { return m_HostName; };

      void SetFullName(const std::string& _value) { m_FullName = _value; };
      const std::string& GetFullName() const { return m_FullName; };

      void SetIP(const int _value) { m_IP = _value; };
      const int GetIP() const { return m_IP; };

      void SetPort(const int _value) { m_Port = _value; };
      const int GetPort() const { return m_Port; };
  }; // LookupRecord

  /**
	  @author Patrick Staehlin <me@packi.ch>
  */
  class BonjourHandler : public Thread {
    private:
      DNSServiceRef m_RegisterReference;
    public:
      BonjourHandler();
      virtual ~BonjourHandler();

      virtual void Execute();

  }; //  Bonjour

}

#endif
