#pragma once

#if defined __GNUC__
  #if __GNUC__ <5 && ! __clang__
    // OE_CORE version
    #define DS_OVERRIDE
    #define DS_NULLPTR 0
  #endif
#endif
#ifndef DS_OVERRIDE
    // C++11 capable version
    #define DS_OVERRIDE override
    #define DS_NULLPTR nullptr
#endif

// define to `[[fallthrough]]` on c++17 complaint compilers
#define DS_FALLTHROUGH

#ifdef __GNUC__
// Branch prediction macros.  Evaluates to the condition given, but also tells the compiler that we
// expect the condition to be true/false enough of the time that it's worth hard-coding branch
// prediction.
#define DS_LIKELY(condition) __builtin_expect(bool(condition), true)
#define DS_UNLIKELY(condition) __builtin_expect(bool(condition), false)
#define DS_NORETURN __attribute__((noreturn))

#else

#define DS_LIKELY(condition) (bool(condition))
#define DS_UNLIKELY(condition) (bool(condition))
#define DS_NORETURN

#endif

/// Macro expanding to `,`. Useful to pass comma to macro without starting next argument
#define DS_COMMA ,

#define DS_WARN_UNUSED_RESULT __attribute__((warn_unused_result))
