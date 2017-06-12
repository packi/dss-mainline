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
#include <sys/time.h>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/foreach.hpp>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <mutex>
#include "common.h"

namespace ds {
namespace log {

static __thread Context* t_context;

Context::Context() : m_next(t_context) {
    t_context = this;
}

Context::~Context() {
    DS_ASSERT(t_context == this);
    t_context = m_next;
}

std::ostream& operator<<(std::ostream& stream, Context* context) {
    for (; context; context = context->m_next) {
        context->ostream(stream);
    }
    return stream;
}

Context* getContext() {
    return t_context;
}

std::ostream& operator<<(std::ostream& stream, Severity x) {
    switch (x) {
        case Severity::PRINT:
            return stream << "!!!!!";
        case Severity::FATAL:
            return stream << "FATAL";
        case Severity::ERROR:
            return stream << "ERROR";
        case Severity::WARNING:
            return stream << "WARN ";
        case Severity::NOTICE:
            return stream << "NOTIC";
        case Severity::INFO:
            return stream << "INFO ";
        case Severity::DEBUG:
            return stream << "DEBUG";
    }
    return stream << "?(" << static_cast<int>(x) << ")";
}

boost::optional<Severity> tryParseSeverityChar(const char* x) {
    if (x[0] && !x[1]) {
        switch (x[0]) {
            case 'F':
                return Severity::FATAL;
            case 'E':
                return Severity::ERROR;
            case 'W':
                return Severity::WARNING;
            case 'N':
                return Severity::NOTICE;
            case 'I':
                return Severity::INFO;
            case 'D':
                return Severity::DEBUG;
        }
    }
    return boost::none;
}

Channel::Channel(const char* name) : m_name(name), m_severity(Severity::NOTICE) {
    Logger::instance().addChannel(*this);
}

Channel::~Channel() {
    Logger::instance().removeChannel(*this);
}

void Channel::log(Severity severity, std::string message) const {
    Logger::instance().log(name(), severity, std::move(message));
}

std::ostream& operator<<(std::ostream& stream, const Channel& x) {
    return stream << x.name() << ':' << x.severity();
}

/// Parse rule, return none on failure
boost::optional<Rule> tryParseRule(char* rule) {
    auto separator = strchr(rule, ':');
    if (!separator) {
        if (auto severity = tryParseSeverityChar(rule)) {
            return Rule{std::string(), *severity};
        }
        return boost::none;
    }

    auto name = std::string(rule, separator);
    for (auto c : name) {
        if (!::isalnum(c)) { // invalid character
            return boost::none;
        }
    }
    if (auto severity = tryParseSeverityChar(separator + 1)) {
        return Rule{std::move(name), *severity};
    }
    return boost::none;
}

std::vector<Rule> tryParseRules(const char* rulesIn) {
    std::vector<Rule> out;
    std::vector<char> rulesCopy(rulesIn, rulesIn + strlen(rulesIn) + 1);
    char* data = rulesCopy.data();
    while (*data) {
        auto ruleEnd = strchr(data, ',');
        if (ruleEnd) {
            *ruleEnd = 0;
        }

        if (auto rule = tryParseRule(data)) {
            out.emplace_back(std::move(*rule));
        }

        if (ruleEnd) {
            data = ruleEnd + 1; // continue with next rule
        } else {
            break;
        }
    }
    return out;
}

struct Logger::Impl {
    std::mutex mutex;
    LogFunction logFunction;
    std::vector<Channel*> channels;
    std::vector<Rule> rules;
};

Logger::Logger() : m_impl(new Impl()) {
    setLogFunction(Logger::defaultLogFunction);
    auto debugEnv = getenv("DS_DEBUG");
    if (debugEnv) {
        m_impl->rules = tryParseRules(debugEnv);
    }
}

Logger& Logger::instance() {
    // static pointer that leaks instance to avoid static
    // deinitialization order fiasco
    static Logger* s_instance = new Logger();
    return *s_instance;
}

void Logger::log(const char* channelName, Severity severity, std::string message) const {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    m_impl->logFunction(channelName, severity, std::move(message));
}

void Logger::setLogFunction(LogFunction logFunction) {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    m_impl->logFunction = std::move(logFunction);
}

void Logger::defaultLogFunction(const char* channelName, Severity severity, std::string message) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm* tm_info = gmtime(&tv.tv_sec);
    char dateTimeStr[26];
    snprintf(dateTimeStr, sizeof(dateTimeStr), "%04d-%02d-%02d %02d:%02d:%02d.%03dZ", 1900 + tm_info->tm_year,
            tm_info->tm_mon + 1, tm_info->tm_mday, tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec,
            int(tv.tv_usec / 1000));
    auto str = ds::str('[', dateTimeStr, "] ", severity, " ", channelName, " ", message, "\n");

    // Use raw file descriptor instead of std::cerr. Logger can be created
    // and called
    // from static initialization code and std::cerr may not be initialized
    // at this point.
    auto size = str.size();
    auto data = str.c_str();
    while (size) {
        auto written = ::write(2, data, size);
        if (written < 0) {
            break;
        }
        size -= written;
        data += written;
    }
}

void Logger::addChannel(Channel& channel) {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    m_impl->channels.emplace_back(&channel);

    auto&& channelName = channel.name();
    BOOST_FOREACH (const auto& rule, m_impl->rules) {
        if (boost::starts_with(channelName, rule.channelNamePrefix)) {
            channel.setSeverity(rule.severity);
        }
    }
}

void Logger::removeChannel(Channel& channel) {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    m_impl->channels.erase(
            std::remove(m_impl->channels.begin(), m_impl->channels.end(), &channel), m_impl->channels.end());
}

namespace _private {

static std::string trimFileToLastSrc(std::string file) {
    auto src = "src/";
    auto srcLen = ::strlen(src);
    auto pos = file.rfind(src);
    if (pos != std::string::npos) {
        file.erase(0, pos + srcLen);
    }
    return file;
}

static std::string trimFileWithDotsToMaxSize(std::string file) {
    constexpr auto maxSize = 32;
    constexpr auto dotsSize = 3;
    if (file.size() <= maxSize) {
        return file;
    }
    file.erase(0, file.size() - maxSize);
    DS_ASSERT(file.size() == maxSize);

    for (int i = 0; i < dotsSize; i++) {
        file[i] = '.';
    }
    return file;
}

std::string trimFile(std::string file) {
    return trimFileWithDotsToMaxSize(trimFileToLastSrc(std::move(file)));
}

void assert_(const std::string& x) {
    std::cerr << x << std::endl;
    abort();
}

} // namespace _private

} // namespace log
} // namespace ds
