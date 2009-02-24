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

#include <stdint.h>
#include <string>

namespace dss {

  /** Bus id of a device */
  typedef uint16_t devid_t;

  /** DSID of a device/modulator */

  typedef struct dsid{
    uint64_t upper;
    uint32_t lower;

    dsid()
    : upper(0), lower(0) {}

    dsid(uint64_t _upper, uint32_t _lower)
    : upper(_upper), lower(_lower) {}

    bool operator==(const dsid& _other) const {
      return (upper == _other.upper) && (lower == _other.lower);
    }

    dsid& operator=(const dsid& _other) {
      upper = _other.upper;
      lower = _other.lower;
      return *this;
    }

    std::string ToString() const;

    static dsid FromString(const std::string& _string);
  } dsid_t;

  extern const dsid_t NullDSID;

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
