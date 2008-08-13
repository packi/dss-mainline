
namespace dss {
  void* dss_malloc(unsigned int _size);
  void* dss_realloc(void* _data, unsigned int _newSize);
  void dss_free(void* _data);
}
