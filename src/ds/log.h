/*
    Copyright (c) 2016 digitalSTROM.org, Zurich, Switzerland

    This file is part of digitalSTROM Server.

    digitalSTROM Server is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    digitalSTROM Server is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with digitalSTROM Server. If not, see <http://www.gnu.org/licenses/>.
*/
#pragma once

#include <boost/optional.hpp>
#include <memory>
#include <stdexcept>
#include <vector>
#include "str.h"

namespace ds {
namespace log {

/// Logging severities
enum class Severity {
    /// Unconditional debug information. Logging at this level shall not be committed.
    PRINT,

    /// Something went wrong, and execution cannot continue.
    FATAL,

    /// Something is wrong, but execution can continue with garbage output.
    ERROR,

    /// A problem was detected but execution can continue with correct output.
    WARNING,

    /// Important information, does not indicate a problem. Printed by default.
    NOTICE,

    /// Information describing what the code is up to.
    /// E.g enters to public methods.
    /// Not printed by default.
    INFO,

    /// Detailed debug information. E.g. enters to private methods. Not printed by default.
    DEBUG,
};
std::ostream& operator<<(std::ostream& stream, Severity x);
// parses severity at the start of the string
boost::optional<Severity> tryParseSeverityChar(const char*);

/// All logging macros stringify passed expressions together with their value.
/// String literals and expressions starting with char '_' are not stringified.
///
/// Example:
///
///     DS_NOTICE("Connected to server.", address, port);
///
/// This will log message like like:
///
///     Connected to server. address:128.55.2.2 port:844
///
/// Macros `DS_REQUIRE` and `DS_ASSERT` check for preconditions and invariants
/// instead of `throw` and `assert`.
///
/// Macro `DS_PRINT` logs unconditionally without using any channgel (like printf).
/// Such code shall not be committed.
///
/// Macro `DS_STATIC_LOG_CHANNEL` defines default log channel in current
/// compilation unit and makes it available for logging macros
/// `DS_ERROR`, `DS_WARNING`, ... `DS_DEBUG`, `DS_[DEBUG|INFO]_[ENTER|LEAVE]`.
///
/// Macro `DS_CONTEXT` defines context that is attached to messages in current scope.
///
/// All logging channels default to NOTICE severity. Use `DS_LOG` environmental variable to
/// tune channels severity. `F` = FATAL, `E` = ERROR, etc
///
/// `DS_LOG` environmental variable syntax:
///
///    [DEFAULT_SEVERITY],[CHANNEL_NAME_PREFIX1]:[SEVERITY1],[CHANNEL_NAME_PREFIX2]:[SEVERITY2]
///
/// Examples:
/// ````
/// DS_LOG=W ./binary
/// DS_LOG=D ./binary
/// DS_LOG=W,dsAsioWatchdog:D,dsSqlite3:I ./binary
/// ````
///
/// Overload std::ostream << operator in the same namespace as your class
/// to make your class serializable in logging.
///
/// It is good practice to align the channel name with source file name
/// and with namespace + class name.
/// The common prefix with channel name can be then ommitted in the log message
///
/// Example file src/foo/bar.cpp:
/// ````cpp
/// #include "bar.h"
/// #include <ds/log.h>
///
/// DS_STATIC_LOG_CHANNEL(fooBar);
///
/// namespace foo {
/// Bar::Bar() { DS_INFO_ENTER(); }
/// Bar::listen(const char* address, int port) {
///     DS_DEBUG_ENTER(address, port);
///     DS_REQUIRE(port != 0, "Port must be explicit.", address, port);
///     DS_NOTICE("Listening on ", address, port);
/// }
/// ````

/// Provide a for-each construct for variadic macros.
/// Source https://codecraft.co/2014/11/25/variadic-macros-tricks/

// Accept any number of args >= N, but expand to just the Nth one.
// Here, N == 22.
#define DS_GET_NTH_ARG(                                                                                         \
        _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, N, ...) \
    N

// Define some macros to help us create overrides based on the
// arity of a for-each-style macro.
#define DS_MFE_0(_call, ...)
#define DS_MFE_1(_call, x) _call(x)
#define DS_MFE_2(_call, x, ...) _call(x) DS_MFE_1(_call, __VA_ARGS__)
#define DS_MFE_3(_call, x, ...) _call(x) DS_MFE_2(_call, __VA_ARGS__)
#define DS_MFE_4(_call, x, ...) _call(x) DS_MFE_3(_call, __VA_ARGS__)
#define DS_MFE_5(_call, x, ...) _call(x) DS_MFE_4(_call, __VA_ARGS__)
#define DS_MFE_6(_call, x, ...) _call(x) DS_MFE_5(_call, __VA_ARGS__)
#define DS_MFE_7(_call, x, ...) _call(x) DS_MFE_6(_call, __VA_ARGS__)
#define DS_MFE_8(_call, x, ...) _call(x) DS_MFE_7(_call, __VA_ARGS__)
#define DS_MFE_9(_call, x, ...) _call(x) DS_MFE_8(_call, __VA_ARGS__)
#define DS_MFE_10(_call, x, ...) _call(x) DS_MFE_9(_call, __VA_ARGS__)
#define DS_MFE_11(_call, x, ...) _call(x) DS_MFE_10(_call, __VA_ARGS__)
#define DS_MFE_12(_call, x, ...) _call(x) DS_MFE_11(_call, __VA_ARGS__)
#define DS_MFE_13(_call, x, ...) _call(x) DS_MFE_12(_call, __VA_ARGS__)
#define DS_MFE_14(_call, x, ...) _call(x) DS_MFE_13(_call, __VA_ARGS__)
#define DS_MFE_15(_call, x, ...) _call(x) DS_MFE_14(_call, __VA_ARGS__)
#define DS_MFE_16(_call, x, ...) _call(x) DS_MFE_15(_call, __VA_ARGS__)
#define DS_MFE_17(_call, x, ...) _call(x) DS_MFE_16(_call, __VA_ARGS__)
#define DS_MFE_18(_call, x, ...) _call(x) DS_MFE_17(_call, __VA_ARGS__)
#define DS_MFE_19(_call, x, ...) _call(x) DS_MFE_18(_call, __VA_ARGS__)
#define DS_MFE_20(_call, x, ...) _call(x) DS_MFE_19(_call, __VA_ARGS__)

#define DS_MACRO_FOR_EACH(x, ...)                                                                                    \
    DS_GET_NTH_ARG("ignored", ##__VA_ARGS__, DS_MFE_20, DS_MFE_19, DS_MFE_18, DS_MFE_17, DS_MFE_16, DS_MFE_15,       \
            DS_MFE_14, DS_MFE_13, DS_MFE_12, DS_MFE_11, DS_MFE_10, DS_MFE_9, DS_MFE_8, DS_MFE_7, DS_MFE_6, DS_MFE_5, \
            DS_MFE_4, DS_MFE_3, DS_MFE_2, DS_MFE_1, DS_MFE_0)                                                        \
    (x, ##__VA_ARGS__)

/// Return passed stringified argument if it is worth to log or an empty string othwerwise.
///
/// Example:
/// trimArgName(##hello) -> "hello"
/// trimArgName(##"hello") -> ""
constexpr const char* trimArgName(const char* x) {
    return (x[0] != '"') && (x[0] != '_') ? x : "";
}

/// `DS_CONTEXT(...)`:  Adds additional contextual information
/// to logs and exceptions thrown from within the current scope.
/// Note that the parameters to DS_CONTEXT() are evaluated lazily,
/// any variables used must remain valid until the end of the scope.
///
#define DS_CONTEXT(...)                                                                                    \
    auto DS_UNIQUE_NAME(_dsContextFunc) = [&](std::ostream& ostream) {                                     \
        ::ds::_private::strRecursive(ostream, "" DS_MACRO_FOR_EACH(DS_LOG_ARG, ##__VA_ARGS__));            \
    };                                                                                                     \
    ::ds::log::_private::ContextImpl<decltype(DS_UNIQUE_NAME(_dsContextFunc))> DS_UNIQUE_NAME(_dsContext)( \
            DS_UNIQUE_NAME(_dsContextFunc))

/// Linked list of thread local contexts.
/// Use `DS_CONTEXT` macro to make new context in current scope
/// Use `getContext()` to get current context.
class Context : boost::noncopyable {
protected:
    /// Adds the instance to the thread local linked list of contexts
    Context();

    /// Removes the instance from the thread local linked list of contexts
    virtual ~Context();

    // Serializes this context item
    virtual void ostream(std::ostream&) = 0;

private:
    Context* m_next;
    friend std::ostream& operator<<(std::ostream&, Context*);
};
std::ostream& operator<<(std::ostream&, Context*);

/// Get current context. Can be serialized by `ostream <<`.
Context* getContext();

#define DS_LOG_ARG(x) , ::ds::log::trimArgName(#x ":"), x, " "
#define DS_LOG_CONTEXT ::ds::log::_private::trimFile(__FILE__), ':', __LINE__, ' ', ::ds::log::getContext()
#define DS_LOG_CHANNEL_CONTEXT(channel) \
    ::ds::log::_private::trimFile(__FILE__, channel), ':', __LINE__, ' ', ::ds::log::getContext()

// * `DS_REQUIRE(condition, ...)`:  Check external input and preconditions
// e.g. to validate parameters passed from a caller.
// A failure indicates that the caller is buggy.
//
// Compared to an explicit `throw` clause, this macro captures more context
// and enforces the `ds` exception policy.
//
// Example:
//
// DS_REQUIRE(value >= 0, "Value cannot be negative.", value);
// DS_FAIL_REQUIRE("Unknown", value);
//
#define DS_REQUIRE(condition, ...)                                                                                    \
    do {                                                                                                              \
        if (DS_LIKELY(condition)) {                                                                                   \
        } else {                                                                                                      \
            throw std::runtime_error(::ds::log::str(                                                                  \
                    "" DS_MACRO_FOR_EACH(DS_LOG_ARG, ##__VA_ARGS__), "condition:", #condition, ' ', DS_LOG_CONTEXT)); \
        }                                                                                                             \
    } while (0)
#define DS_FAIL_REQUIRE(...) \
    throw std::runtime_error(::ds::log::str("" DS_MACRO_FOR_EACH(DS_LOG_ARG, ##__VA_ARGS__), DS_LOG_CONTEXT))

// Assert the `condition` is true.
// This macro should be used to check for bugs in the surrounding code (broken invariant, ...)
// and its dependencies, but NOT to check for invalid input.
// Example:
//
// DS_ASSERT(m_state == State::CONNECTED, m_state);
// DS_FAIL_ASSERT("Invalid", m_state);
//
#define DS_ASSERT(condition, ...)                                                                                   \
    do {                                                                                                            \
        if (DS_LIKELY(condition)) {                                                                                 \
        } else {                                                                                                    \
            ::ds::log::_private::assert_(                                                                           \
                    ::ds::log::str("Assertion failed. " DS_MACRO_FOR_EACH(DS_LOG_ARG, ##__VA_ARGS__), "condition:", \
                            #condition, ' ', DS_LOG_CONTEXT));                                                      \
        }                                                                                                           \
    } while (0)

#define DS_FAIL_ASSERT(...)       \
    ::ds::log::_private::assert_( \
            ::ds::log::str("Assertion failed. " DS_MACRO_FOR_EACH(DS_LOG_ARG, ##__VA_ARGS__), DS_LOG_CONTEXT))

// Logging channel registered into Logger singleton.
//
// Use DS_STATIC_LOG_CHANNEL to define a default static instance
// for use by DS_DEBUG, DS_INFO, etc macros.
//
// Make your own instance for use
// DS_CHANNEL_DEBUG, DS_CHANNEL_INFO, etc. macros
class Channel : boost::noncopyable {
public:
    Channel(const char* name);
    ~Channel();

    const char* name() const { return m_name; }

    Severity severity() const { return m_severity; }
    void setSeverity(Severity x) { m_severity = x; }

    bool shouldLog(Severity severity) const { return severity <= m_severity; }
    void log(Severity severity, std::string message) const;

private:
    // copy of severity from channel to make method `shouldLog`
    // method as fast as possible.
    const char* m_name;
    Severity m_severity;
};
std::ostream& operator<<(std::ostream& stream, const Channel& x);

/// Rule specifying severity for all channels
/// with name starting with given prefix.
struct Rule {
    std::string channelNamePrefix;
    Severity severity;
    bool operator==(const Rule& x) const { return channelNamePrefix == x.channelNamePrefix && severity == x.severity; }
};

/// Parse rule in format "channelNamePrefix:severity" / "severity"
boost::optional<Rule> tryParseRule(char* rule);

/// Parse rules in format "severity,channelNamePrefix1:severity,channelNamePrefix2:severity"
/// Parsing failures are ignored.
std::vector<Rule> tryParseRules(const char*);

/// Logger singleton holding reference to all channels.
/// Methods are thread safe.
class Logger : boost::noncopyable {
    Logger();

public:
    static Logger& instance();

    typedef std::function<void(const char* channelName, Severity severity, std::string)> LogFunction;

    // Synchronize to internal mutex and log the message by calling `logFunction`.
    void log(const char* channelName, Severity severity, std::string) const;

    /// Set log function called by `log()`.
    void setLogFunction(LogFunction);

    /// Default log function writing messages to error output
    static void defaultLogFunction(const char* channelName, Severity severity, std::string);

    // No getter for channels.
    // Iteration over channels would require clients to hold our mutex,
    // because channels are not copyable
    // and can be added and removed at any time.
    // But we may want to introduce query
    // for list of all channelNames with their severity.

private:
    struct Impl;
    const std::unique_ptr<Impl> m_impl;

    // adds channel and sets its severity
    void addChannel(Channel& channel);
    void removeChannel(Channel& channel);

    friend class Channel;
};

/// Define default static logger for use in
#define DS_STATIC_LOG_CHANNEL_IDENTIFIER _dsChannel
#define DS_STATIC_LOG_CHANNEL(name) static ::ds::log::Channel DS_STATIC_LOG_CHANNEL_IDENTIFIER(#name)

#define DS_CHANNEL_LOG(channel, severity, ...)                                                                 \
    if (!channel.shouldLog(::ds::log::Severity::severity)) {                                                   \
    } else {                                                                                                   \
        channel.log(::ds::log::Severity::severity,                                                             \
                ::ds::log::str(DS_LOG_CHANNEL_CONTEXT(channel) DS_MACRO_FOR_EACH(DS_LOG_ARG, ##__VA_ARGS__))); \
    }

#define DS_CHANNEL_DEBUG(channel, ...) DS_CHANNEL_LOG(channel, DEBUG, ##__VA_ARGS__)
#define DS_CHANNEL_DEBUG_ENTER(channel, ...) DS_CHANNEL_LOG(channel, DEBUG, "Enter", __FUNCTION__, ##__VA_ARGS__)
#define DS_CHANNEL_DEBUG_LEAVE(channel, ...) DS_CHANNEL_LOG(channel, DEBUG, "Leave", __FUNCTION__, ##__VA_ARGS__)
#define DS_CHANNEL_INFO(channel, ...) DS_CHANNEL_LOG(channel, INFO, ##__VA_ARGS__)
#define DS_CHANNEL_INFO_ENTER(channel, ...) DS_CHANNEL_LOG(channel, INFO, "Enter", __FUNCTION__, ##__VA_ARGS__)
#define DS_CHANNEL_INFO_LEAVE(channel, ...) DS_CHANNEL_LOG(channel, INFO, "Leave", __FUNCTION__, ##__VA_ARGS__)
#define DS_CHANNEL_NOTICE(channel, ...) DS_CHANNEL_LOG(channel, NOTICE, ##__VA_ARGS__)
#define DS_CHANNEL_WARNING(channel, ...) DS_CHANNEL_LOG(channel, WARNING, ##__VA_ARGS__)
#define DS_CHANNEL_ERROR(channel, ...) DS_CHANNEL_LOG(channel, ERROR, ##__VA_ARGS__)
#define DS_CHANNEL_FATAL(channel, ...) DS_CHANNEL_LOG(channel, FATAL, ##__VA_ARGS__)

// Macros logging to channel defined by `DS_STATIC_LOG_CHANNEL`
#define DS_LOG(...) DS_CHANNEL_LOG(DS_STATIC_LOG_CHANNEL_IDENTIFIER, ##__VA_ARGS__)
#define DS_DEBUG(...) DS_CHANNEL_DEBUG(DS_STATIC_LOG_CHANNEL_IDENTIFIER, ##__VA_ARGS__)
#define DS_DEBUG_ENTER(...) DS_CHANNEL_DEBUG_ENTER(DS_STATIC_LOG_CHANNEL_IDENTIFIER, ##__VA_ARGS__)
#define DS_DEBUG_LEAVE(...) DS_CHANNEL_DEBUG_LEAVE(DS_STATIC_LOG_CHANNEL_IDENTIFIER, ##__VA_ARGS__)
#define DS_INFO(...) DS_CHANNEL_INFO(DS_STATIC_LOG_CHANNEL_IDENTIFIER, ##__VA_ARGS__)
#define DS_INFO_ENTER(...) DS_CHANNEL_INFO_ENTER(DS_STATIC_LOG_CHANNEL_IDENTIFIER, ##__VA_ARGS__)
#define DS_INFO_LEAVE(...) DS_CHANNEL_INFO_LEAVE(DS_STATIC_LOG_CHANNEL_IDENTIFIER, ##__VA_ARGS__)
#define DS_NOTICE(...) DS_CHANNEL_NOTICE(DS_STATIC_LOG_CHANNEL_IDENTIFIER, ##__VA_ARGS__)
#define DS_WARNING(...) DS_CHANNEL_WARNING(DS_STATIC_LOG_CHANNEL_IDENTIFIER, ##__VA_ARGS__)
#define DS_ERROR(...) DS_CHANNEL_ERROR(DS_STATIC_LOG_CHANNEL_IDENTIFIER, ##__VA_ARGS__)
#define DS_FATAL(...) DS_CHANNEL_FATAL(DS_STATIC_LOG_CHANNEL_IDENTIFIER, ##__VA_ARGS__)

// * DS_PRINT(...)`:  print temporary log messsage unconditionally.
// Useful for ad-hoc logging. Code with this macro shall not be committed.
#define DS_PRINT(...)                                                 \
    ::ds::log::Logger::instance().log("", ::ds::log::Severity::PRINT, \
            ::ds::log::str(DS_LOG_CONTEXT DS_MACRO_FOR_EACH(DS_LOG_ARG, ##__VA_ARGS__)))

/// Stringify function tuned for logging.
template <typename... Args>
std::string str(Args&&... args) {
    auto s = ::ds::str(std::forward<Args>(args)...);
    if (auto size = s.size()) {
        if (s[size - 1] == ' ') {
            s.resize(size - 1); // last character is extra ' ' due to macro expansion problems
        }
    }
    return s;
}

namespace _private {
/// Trim file for logging purposes.
std::string trimFile(std::string file);
std::string trimFile(std::string file, const ds::log::Channel& channel);

template <typename Func>
class ContextImpl : public Context {
public:
    ContextImpl(Func& func) : m_func(func) {}

private:
    Func& m_func;
    void ostream(std::ostream& s) DS_OVERRIDE { m_func(s); }
};

/// Assert (abort or throw) with given message.
///
/// assert is macro, so we cannot reuse its name here
DS_NORETURN void assert_(const std::string& message);

} // namespace _private

} // namespace log
} // namespace ds
