#pragma once

#if defined __GNUC__
  #if __GNUC__ <5
    #define DS_OVERRIDE
  #endif
#endif
#ifndef DS_OVERRIDE
    #define DS_OVERRIDE override
#endif

// define to `[[fallthrough]]` on c++17 complaint compilers
#define DS_FALLTHROUGH

#ifdef __GNUC__
#define DS_LIKELY(condition) __builtin_expect(condition, true)
#define DS_UNLIKELY(condition) __builtin_expect(condition, false)
// Branch prediction macros.  Evaluates to the condition given, but also tells the compiler that we
// expect the condition to be true/false enough of the time that it's worth hard-coding branch
// prediction.
#else
#define KJ_LIKELY(condition) (condition)
#define KJ_UNLIKELY(condition) (condition)
#endif