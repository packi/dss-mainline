
#include "series.h"

namespace dss {

  //================================================== Value

#ifdef WITH_GCOV
  template<class T>
  Series<T>::Series() {}

  AdderValue::AdderValue() {}
  CurrentValue::CurrentValue() {};
  Value::Value() {};
#endif

} // namespace dss
