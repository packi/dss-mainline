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
#include "log.h"
#include <ds/catch/catch.h>
#include <boost/optional/optional_io.hpp>
#include <thread>

static const char* TAGS = "[dsLog][ds]";

TEST_CASE("dsLogTrimArgName", TAGS) {
    SECTION("discards string constants") { CHECK(ds::str(ds::log::trimArgName("\"str\":")) == ""); }
    SECTION("passes variable names") { CHECK(ds::str(ds::log::trimArgName("variable:")) == "variable:"); }
    SECTION("discard variables starting with '_'") { CHECK(ds::str(ds::log::trimArgName("_variable:")) == ""); }
}

TEST_CASE("DS_CONTEXT", TAGS) {
    SECTION("is empty at start") { CHECK(ds::str(ds::log::getContext()) == ""); }

    SECTION("is stacked and scope limited") {
        DS_CONTEXT("a");
        CHECK(ds::str(ds::log::getContext()) == "a ");
        {
            int x = 7;
            DS_CONTEXT(x);
            CHECK(ds::str(ds::log::getContext()) == "x:7 a ");
        }
        CHECK(ds::str(ds::log::getContext()) == "a ");
    }

    SECTION("is thread local") {
        DS_CONTEXT("a");
        std::thread thread([&]() {
            DS_CONTEXT("b");
            CHECK(ds::str(ds::log::getContext()) == "b ");
        });
        thread.join();
        CHECK(ds::str(ds::log::getContext()) == "a ");
    }
}

TEST_CASE("DS_REQUIRE", TAGS) {
    SECTION("does nothing on success") { DS_REQUIRE(true, ""); }

    SECTION("accepts context arguments on success but does not evaluate them") {
        bool evaluated = false;
        auto evaluate = [&]() {
            evaluated = true;
            return 0;
        };
        DS_CONTEXT(evaluate());
        int x = 5;
        DS_REQUIRE(true, "context string", x, evaluate());
        CHECK(!evaluated);
    }

    SECTION("throws on failure") { CHECK_THROWS(DS_REQUIRE(false, "")); }

    SECTION("captures context on failure") {
        auto foo = 7;
        DS_CONTEXT(foo);
        int line = 0;
        auto fail = [&] {
            int a = 1;
            line = __LINE__ + 1; // must be one line before DS_REQUIRE macro
            DS_REQUIRE(a == 4, "Variable a was compared to 4.", a);
        };
        CHECK_THROWS_FIND(
                fail(), ds::str("Variable a was compared to 4. a:1 condition:a == 4 ds/log-test.cpp:", line, " foo:7"));
    }
}

TEST_CASE("DS_FAIL_REQUIRE", TAGS) {
    SECTION("throws on failure") { CHECK_THROWS(DS_FAIL_REQUIRE("")); }

    SECTION("captures context on failure") {
        auto foo = 7;
        DS_CONTEXT(foo);
        int a = 1;
        auto line = __LINE__ + 1; // must be one line before DS_FAIL_REQUIRE macro
        CHECK_THROWS_FIND(DS_FAIL_REQUIRE("Failed.", a), ds::str("Failed. a:1 ds/log-test.cpp:", line, " foo:7"));
    }
    int x = 5;
    SECTION("throws") {
        int line = 0;
        auto f = [&] {
            line = __LINE__ + 1;
            DS_FAIL_REQUIRE("Invalid x.", x);
        };
        CHECK_THROWS_FIND(f(), ds::str("Invalid x. x:5 ds/log-test.cpp:", line));
    }
}

TEST_CASE("DS_ASSERT", TAGS) {
    SECTION("does nothing on success") { DS_ASSERT(true, ""); }

    SECTION("accepts context arguments on success but does not evaluate them") {
        bool evaluated = false;
        auto evaluate = [&]() {
            evaluated = true;
            return 0;
        };
        DS_CONTEXT(evaluate());
        int x = 5;
        DS_ASSERT(true, x, evaluate());
        CHECK(!evaluated);
    }
}

TEST_CASE("DS_FAIL_ASSERT", TAGS) {
    SECTION("compiles") {
        if (0) {
            // TODO(someday): error with g++ (Ubuntu 5.4.0-6ubuntu1~16.04.4) 5.4.0 20160609
            // In file included from ../../build/../src/ds/log-test.cpp:19:0:
            // ../../build/../src/ds/log-test.cpp: In function ‘void ____C_A_T_C_H____T_E_S_T____62()’:
            // ../../build/../src/ds/log.h:75:54: error: expected primary-expression before ‘,’ token
            //  #define DS_LOG_ARG(x) , ::ds::log::trimArg(#x ":"), x, " "
            //                                                       ^
            // ../../build/../src/ds/log.h:36:28: note: in expansion of macro ‘DS_LOG_ARG’
            //  #define DS_MFE_1(_call, x) _call(x)
            //                             ^
            // ../../build/../src/ds/log.h:31:5: note: in expansion of macro ‘DS_MFE_1’
            //      N
            //      ^
            // ../../build/../src/ds/log.h:116:45: note: in expansion of macro ‘DS_MACRO_FOR_EACH’
            //      ::ds::log::_private::assert_(ds::str("" DS_MACRO_FOR_EACH(DS_LOG_ARG, ##__VA_ARGS__),
            //      DS_LOG_FILE_CONTEXT))
            //                                              ^
            // ../../build/../src/ds/log-test.cpp:65:13: note: in expansion of macro ‘DS_FAIL_ASSERT’
            //
            // DS_FAIL_ASSERT();
        }
    }
    SECTION("compiles with context") {
        auto foo = 7;
        DS_CONTEXT(foo);
        int a = 1;
        if (0) {
            DS_FAIL_ASSERT(a);
        }
    }
}

TEST_CASE("tryParseSeverityChar", TAGS) {
    SECTION("Severity first uppercase letter is parsed") {
        CHECK(ds::log::tryParseSeverityChar("F") == ds::log::Severity::FATAL);
        CHECK(ds::log::tryParseSeverityChar("E") == ds::log::Severity::ERROR);
        CHECK(ds::log::tryParseSeverityChar("W") == ds::log::Severity::WARNING);
        CHECK(ds::log::tryParseSeverityChar("N") == ds::log::Severity::NOTICE);
        CHECK(ds::log::tryParseSeverityChar("I") == ds::log::Severity::INFO);
        CHECK(ds::log::tryParseSeverityChar("D") == ds::log::Severity::DEBUG);
    }
    SECTION("Severity first lowercase letter is not parsed") {
        CHECK(ds::log::tryParseSeverityChar("f") == boost::none);
        CHECK(ds::log::tryParseSeverityChar("e") == boost::none);
        CHECK(ds::log::tryParseSeverityChar("w") == boost::none);
        CHECK(ds::log::tryParseSeverityChar("n") == boost::none);
        CHECK(ds::log::tryParseSeverityChar("i") == boost::none);
        CHECK(ds::log::tryParseSeverityChar("d") == boost::none);
    }
    SECTION("Full names, weird strings are not parsed") {
        CHECK(ds::log::tryParseSeverityChar("") == boost::none);
        CHECK(ds::log::tryParseSeverityChar("FATAL") == boost::none);
        CHECK(ds::log::tryParseSeverityChar("ERROR") == boost::none);
        CHECK(ds::log::tryParseSeverityChar("?") == boost::none);
    }
}

TEST_CASE("severityFromSyslog", TAGS) {
    CHECK(ds::log::severityFromSyslog(0) == ds::log::Severity::FATAL);
    CHECK(ds::log::severityFromSyslog(1) == ds::log::Severity::FATAL);
    CHECK(ds::log::severityFromSyslog(2) == ds::log::Severity::FATAL);
    CHECK(ds::log::severityFromSyslog(3) == ds::log::Severity::ERROR);
    CHECK(ds::log::severityFromSyslog(4) == ds::log::Severity::WARNING);
    CHECK(ds::log::severityFromSyslog(5) == ds::log::Severity::NOTICE);
    CHECK(ds::log::severityFromSyslog(6) == ds::log::Severity::INFO);
    CHECK(ds::log::severityFromSyslog(7) == ds::log::Severity::DEBUG);
    CHECK(ds::log::severityFromSyslog(8) == ds::log::Severity::DEBUG);
    CHECK(ds::log::severityFromSyslog(100) == ds::log::Severity::DEBUG);
}

namespace {
class MockedLoggerLogFunction : boost::noncopyable {
public:
    MockedLoggerLogFunction(ds::log::Logger::LogFunction logFunction) {
        ds::log::Logger::instance().setLogFunction(std::move(logFunction));
    }

    ~MockedLoggerLogFunction() { ds::log::Logger::instance().setLogFunction(ds::log::Logger::defaultLogFunction); }
};

} // namespace

TEST_CASE("dsLogChannel", TAGS) {
    SECTION("Default constructed channel") {
        ds::log::Channel channel("dsLogChannelTest");
        CHECK(channel.name() == std::string("dsLogChannelTest"));
        CHECK(channel.severity() == ds::log::Severity::NOTICE);
    }

    SECTION("Severity can be changed") {
        ds::log::Channel channel("dsLogChannelTest");
        channel.setSeverity(ds::log::Severity::WARNING);
        CHECK(channel.severity() == ds::log::Severity::WARNING);
    }

    SECTION("shouldLog compares stored severity with passed one") {
        ds::log::Channel channel("dsLogChannelTest");
        CHECK(channel.shouldLog(ds::log::Severity::FATAL) == true);
        CHECK(channel.shouldLog(ds::log::Severity::ERROR) == true);
        CHECK(channel.shouldLog(ds::log::Severity::WARNING) == true);
        CHECK(channel.shouldLog(ds::log::Severity::NOTICE) == true);
        CHECK(channel.shouldLog(ds::log::Severity::INFO) == false);
        CHECK(channel.shouldLog(ds::log::Severity::DEBUG) == false);
    }

    SECTION("log() is forwarded to Logger::log()") {
        bool called = true;
        MockedLoggerLogFunction logFunction(
                [&](const char* channelName, ds::log::Severity severity, std::string message) {
                    CHECK(channelName == std::string("dsLogChannelTest"));
                    CHECK(severity == ds::log::Severity::INFO);
                    CHECK(message == "message");
                });
        ds::log::Channel channel("dsLogChannelTest");
        channel.log(ds::log::Severity::INFO, "message");
        CHECK(called);
    }
}

TEST_CASE("dsLogParseRules", TAGS) {
    using namespace ds::log;

    SECTION("Parse global rules") {
        CHECK(tryParseRules("D") == std::vector<Rule>({Rule({"", Severity::DEBUG})}));
        CHECK(tryParseRules("W") == std::vector<Rule>({Rule({"", Severity::WARNING})}));
        CHECK(tryParseRules("W,E") == std::vector<Rule>({Rule({"", Severity::WARNING}), Rule({"", Severity::ERROR})}));
    }

    SECTION("Parse channel rules") {
        CHECK(tryParseRules("foo:D") == std::vector<Rule>({Rule({"foo", Severity::DEBUG})}));
    }
    SECTION("Parse global and channel rules") {
        CHECK(tryParseRules("W,foo:D") ==
                std::vector<Rule>({Rule({"", Severity::WARNING}), Rule({"foo", Severity::DEBUG})}));
    }
    SECTION("Channel without severity is not allowed") { CHECK(tryParseRules("foo:") == std::vector<Rule>()); }
    SECTION("Non-ascii charactes in channel name are not allowed") {
        CHECK(tryParseRules("foo+:D") == std::vector<Rule>());
    }
    SECTION("Invalid rules are scipped") {
        CHECK(tryParseRules("I,foo+:D,goo:D") ==
                std::vector<Rule>({Rule({"", Severity::INFO}), Rule({"goo", Severity::DEBUG})}));
    }
}

TEST_CASE("dsLogDefaultChannel macros", TAGS) {
    auto functionName = ds::str("void ____C_A_T_C_H____T_E_S_T____", __LINE__ - 1, "()");
    DS_STATIC_LOG_CHANNEL(dsLogDefaultChannelTest);

    CHECK(DS_STATIC_LOG_CHANNEL_IDENTIFIER.name() == std::string("dsLogDefaultChannelTest"));
    CHECK(DS_STATIC_LOG_CHANNEL_IDENTIFIER.severity() == ds::log::Severity::NOTICE);

    ds::log::Severity lastSeverity;
    std::string lastMessage;
    bool called = false;
    MockedLoggerLogFunction logFunction([&](const char* channelName, ds::log::Severity severity, std::string message) {
        CHECK(channelName == std::string("dsLogDefaultChannelTest"));
        lastSeverity = severity;
        lastMessage = std::move(message);
        called = true;
    });
#define CASE(severity, message, code)                   \
    code;                                               \
    CHECK(called);                                      \
    called = false;                                     \
    CHECK(lastSeverity == ds::log::Severity::severity); \
    CHECK(lastMessage == ds::str("test.cpp:", __LINE__, " y:2 ", message));

#define CASE_NOLOG(code) \
    code;                \
    CHECK(!called);

    int x = 1;
    int y = 2;
    DS_CONTEXT(y);

    DS_STATIC_LOG_CHANNEL_IDENTIFIER.setSeverity(ds::log::Severity::DEBUG);
    CASE(DEBUG, "hello x:1", DS_DEBUG("hello", x));
    CASE(DEBUG, ds::str("Enter ", functionName, " x:1"), DS_DEBUG_ENTER(x));
    CASE(DEBUG, ds::str("Leave ", functionName, " x:1"), DS_DEBUG_LEAVE(x));
    CASE(INFO, "hello x:1", DS_INFO("hello", x));
    CASE(INFO, ds::str("Enter ", functionName, " x:1"), DS_INFO_ENTER(x));
    CASE(INFO, ds::str("Leave ", functionName, " x:1"), DS_INFO_LEAVE(x));
    CASE(NOTICE, "hello x:1", DS_NOTICE("hello", x));
    CASE(WARNING, "hello x:1", DS_WARNING("hello", x));
    CASE(ERROR, "hello x:1", DS_ERROR("hello", x));
    CASE(FATAL, "hello x:1", DS_FATAL("hello", x));

    DS_STATIC_LOG_CHANNEL_IDENTIFIER.setSeverity(ds::log::Severity::WARNING);
    CASE_NOLOG(DS_DEBUG("hello", x));
    CASE_NOLOG(DS_INFO("hello", x));
    CASE_NOLOG(DS_NOTICE("hello", x));
    CASE(WARNING, "hello x:1", DS_WARNING("hello", x));
    CASE(ERROR, "hello x:1", DS_ERROR("hello", x));
    CASE(FATAL, "hello x:1", DS_FATAL("hello", x));

#undef CASE
#undef CASE_NOLOG
}

TEST_CASE("DS_PRINT", TAGS) {
    std::string expectedMessage;
    bool called = false;
    MockedLoggerLogFunction logFunction([&](const char* channelName, ds::log::Severity severity, std::string message) {
        CHECK(channelName == std::string(""));
        CHECK(severity == ds::log::Severity::PRINT);
        CHECK(message == expectedMessage);
        called = true;
    });
    expectedMessage = ds::str("ds/log-test.cpp:", __LINE__ + 1, " hello");
    DS_PRINT("hello");
    CHECK(called);
}

TEST_CASE("dsLogStr", TAGS) {
    SECTION("stringifies") {
        CHECK(ds::log::str() == "");
        CHECK(ds::log::str("foo", "goo", 8) == "foogoo8");
    }
    SECTION("strips last space") {
        CHECK(ds::log::str(" ") == "");
        CHECK(ds::log::str("foo ") == "foo");
    }
}

TEST_CASE("dsLogTrimFile", TAGS) {
    SECTION("Do not trim files bellow 32 characters without src/") {
        CHECK(ds::log::_private::trimFile("") == "");
        CHECK(ds::log::_private::trimFile("a") == "a");
        CHECK(ds::log::_private::trimFile("foo.h") == "foo.h");
        CHECK(ds::log::_private::trimFile("foo.cpp") == "foo.cpp");
        CHECK(ds::log::_private::trimFile("1234567890123456789012345678901") == "1234567890123456789012345678901");
        CHECK(ds::log::_private::trimFile("12345678901234567890123456789012") == "12345678901234567890123456789012");
    }

    SECTION("Trim up to last src/") {
        CHECK(ds::log::_private::trimFile("src/foo.cpp") == "foo.cpp");
        CHECK(ds::log::_private::trimFile("../src/foo.cpp") == "foo.cpp");
        CHECK(ds::log::_private::trimFile("src/src/foo.cpp") == "foo.cpp");
        CHECK(ds::log::_private::trimFile("123456789012345678901234567890123/src/foo.cpp") == "foo.cpp");
    }

    SECTION("Trim with dots to 32 characters") {
        CHECK(ds::log::_private::trimFile("123456789012345678901234567890123") == "...56789012345678901234567890123");
        CHECK(ds::log::_private::trimFile("123456789012345678901234567890.cpp") == "...6789012345678901234567890.cpp");
        CHECK(ds::log::_private::trimFile("src/123456789012345678901234567890.cpp") ==
                "...6789012345678901234567890.cpp");
    }
}

TEST_CASE("Logger.defaultSeverity", TAGS) {
    ds::log::Channel channel("dsLogLoggerSetDefaultSeverity");
    auto&& logger = ds::log::Logger::instance();

    SECTION("defaults to NOTICE") {
        CHECK(logger.defaultSeverity() == ds::log::Severity::NOTICE);
        CHECK(channel.severity() == ds::log::Severity::NOTICE);
    }

    SECTION("can be set") {
        auto oldSeverity = logger.defaultSeverity();
        {
            DS_DEFER(logger.setDefaultSeverity(oldSeverity));
            logger.setDefaultSeverity(ds::log::Severity::DEBUG);
            CHECK(channel.severity() == ds::log::Severity::DEBUG);
        }
        CHECK(channel.severity() == oldSeverity);
    }
}
