#include <stdlib.h>

namespace dss {
  void* dss_malloc(unsigned int _size) {
    return ::malloc(_size);
  }

  void* dss_realloc(void* _data, unsigned int _newSize) {
    return ::realloc(_data, _newSize);
  }

  void dss_free(void* _data) {
    ::free(_data);
  }
}
