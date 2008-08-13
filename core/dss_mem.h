
#ifndef __DSS_MEM_H__
#define __DSS_MEM_H__

#include <dss_defs.h>

#ifndef __PARADIGM__
  #define huge
#endif

void huge *dss_memmove(void huge *dst, void huge *src, ulong n);
void huge *dss_malloc(ulong size);
void huge *dss_realloc(void huge *ptr, ulong size);
bool       dss_free(void huge *ptr);

#endif
