#pragma once

#include <boost/foreach.hpp>
#include <boost/noncopyable.hpp>
#include <utility>

namespace ds {

#if defined __GNUC__
#if __GNUC__ < 5 && !__clang__
// OE_CORE version
#define DS_OVERRIDE
#define DS_FINAL
#define DS_NULLPTR 0
#define DS_CONSTEXPR

// gcc 4.5 does not support explicit default constructor and default move asignment
// We want explicit copy constructor mainly to catch unintended copies.
// If it compiles on new compiler, gcc4.5 will not copy either.
#define DS_MOVABLE_AND_EXPLICIT_COPYABLE(X)

#endif
#endif
#ifndef DS_OVERRIDE
// C++11 capable version
#define DS_OVERRIDE override
#define DS_FINAL final
#define DS_NULLPTR nullptr
#define DS_CONSTEXPR constexpr

// Add explicit copy constructor and move constructor, move assignment
// We want explicit copy constructor mainly to catch unintended copies.
#define DS_MOVABLE_AND_EXPLICIT_COPYABLE(X) \
    explicit X(const X &) = default;        \
    X(X &&) = default;                      \
    X &operator=(X &&) = default;

#endif

/// define to `[[fallthrough]]` on c++17 complaint compilers
#define DS_FALLTHROUGH

#ifdef __GNUC__
/// Branch prediction macros.  Evaluates to the condition given, but also tells the compiler that we
/// expect the condition to be true/false enough of the time that it's worth hard-coding branch
/// prediction.
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

#define DS_CONCAT_(x, y) x##y

/// Concatenate two identifiers into one
#define DS_CONCAT(x, y) DS_CONCAT_(x, y)

/// Create a unique identifier name.
/// We use concatenate __LINE__ rather than __COUNTER__ so that
/// the name can be used multiple times in the same macro
/// and evaluate the same.
/// If the code relies on DS_UNIQUE_NAME generating the same
/// name multiple times, wrap the code into macro to force
/// compilation into one line.
#define DS_UNIQUE_NAME(prefix) DS_CONCAT(prefix, __LINE__)

// Run the given code when the function exits, whether by return or exception.
#define DS_DEFER(code) auto DS_UNIQUE_NAME(_dsDefer) = ::ds::defer([&]() { code; })

namespace _private {
template <typename Func>
class Deferred : boost::noncopyable {
public:
    Deferred(Func&& func) : m_func(std::forward<Func>(func)), m_canceled(false) {}
    ~Deferred() {
        if (!m_canceled)
            m_func();
    }
    Deferred(Deferred&& x) : m_func(std::move(x.m_func)), m_canceled(false) { x.m_canceled = true; }

private:
    Func m_func;
    bool m_canceled;
};
} // namespace _private

// Returns an object which will invoke the given functor in its destructor.  The object is not
// copyable but is movable with the semantics you'd expect.  Since the return type is private,
// you need to assign to an `auto` variable.
//
// The DS_DEFER macro provides slightly more convenient syntax for the common case where you
// want some code to run at current scope exit.
template <typename Func>
_private::Deferred<Func> defer(Func&& func) {
    return _private::Deferred<Func>(std::forward<Func>(func));
}

// Version of KJ_REQUIRE() to be used inside inline methods.
// Does not include the whole logging framework.
#define DS_IREQUIRE(condition, message)        \
    do {                                       \
        if (DS_LIKELY(condition)) {            \
        } else {                               \
            throw std::runtime_error(message); \
        }                                      \
    } while (0)

#define DS_FAIL_IREQUIRE(message) throw std::runtime_error(message)

} // namespace ds
