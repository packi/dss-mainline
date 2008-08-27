/*
 *  ds485types.h
 *  dSS
 *
 *  Created by Patrick Stählin on 4/18/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef _DS485_TYPES_H_INCLUDED
#define _DS485_TYPES_H_INCLUDED

namespace dss {

  typedef unsigned short devid_t;
  typedef char int8;
  typedef unsigned char uint8;
  typedef unsigned long dsid_t;

  typedef enum  {
    Click,
    Touch,
    TouchEnd,
    ShortClick,
    DoubleClick,
    ProgrammClick
  } ButtonPressKind;

}

#endif
