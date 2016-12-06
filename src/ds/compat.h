#pragma once

#if defined __GNUC__
  #if __GNUC__ <5
    #define DS_OVERRIDE
  #endif
#endif
#ifndef DS_OVERRIDE
    #define DS_OVERRIDE override
#endif
