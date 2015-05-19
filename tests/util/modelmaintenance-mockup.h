#ifndef MODELMAINTENANCEMOCKUP_H
#define MODELMAINTENANCEMOCKUP_H

#include "model/modelmaintenance.h"

namespace dss {
class ModelMaintenanceMock : public ModelMaintenance
{
  public:
  ModelMaintenanceMock() : ModelMaintenance(NULL, 2) {
    // allow to add events
    m_IsInitializing = false;
  };

  using ModelMaintenance::handleModelEvents;
};
}

#endif // MODELMAINTENANCEMOCKUP_H
