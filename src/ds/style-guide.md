## Style guide

## Design philosophy

In general consult [KJ Style Guide](https://github.com/sandstorm-io/capnproto/blob/master/style-guide.md)
for design philosophy. But do not fall into `KJ` NIH syndrom. Prefer `std` types for strings, vectors, threads, etc.
Prefer `asio` for networking.

### Prefer Event-Loop paradigm

[KJ:Threads vs. Event Loops](https://github.com/sandstorm-io/capnproto/blob/master/style-guide.md#threads-vs-event-loops)

    Prefer event loop concurrency. Each event callback is effectively a transaction;
    it does not need to worry about concurrent modification within the body of the function.

C++ closures make this model quite usable now (like nodejs).
C++ coroutines will allow to write sequential code in this model as easily as blocking sequential code.

Use single threaded `asio::io_service` for low level event loop implementation.
It will become part of `c++` standard
and has biggest potential to be directly supported in higher level libraries.

### Exceptions - Business logic should never catch

[KJ:Exceptions](https://github.com/sandstorm-io/capnproto/blob/master/style-guide.md#exceptions)

    Business logic should never catch - If you find that callers of your interface
    need to catch and handle certain kinds of exceptions in order to operate correctly,
    then you must change your interface (or overload it) such that those conditions
    can be handled without an exception ever being thrown.

But contrary to kj style: Do not throw in destructors. Do not throw in move constructors.
Declare move constructors `noexcept` to make them usable and effective inside `std::vector`.

### Lazy input validation

[KJ:Lazy input validation](https://github.com/sandstorm-io/capnproto/blob/master/style-guide.md#lazy-input-validation)

    Validation occur lazily, at time of use.
    Code should be written to be tolerant of validation failures.
    For example, most code dealing with UTF-8 text should treat it as a blob of bytes,
    not worrying about invalid byte sequences.
    When you actually need to decode the code points
    -- such as to display them
    -- you should do something reasonable with invalid sequences
    -- such as display the Unicode replacement character.

## C++

### Heap allocations

    Allocation in implicit copies is a common source of death-by-1000-cuts performance problems.

Contrary to `kj` approach, this problem is NOT worth reimplementing the whole standard library.

TODO(someday): Possible solution with `std` types: Use `ds::String`, `ds::Function`, `ds::Vector`,
... that are subclasses of `std::string`, `std::function`, `std::vector`, ...
with explicit copy constructors to make copies visible.
To make this technique effective, all structs needs to follow this convention
and cannot be used together with aggregate initialization.


#### Headers

Use `#pragma once` to prevent multiple inclusion. It is supported on all
[relevant compilers](https://en.wikipedia.org/wiki/Pragma_once#Portability)
and it is less error prone.

Include the header as first `#include` in corresponding `.cpp` file to ensure
that the header is independent.

#### Unit tests

Unit tests are placed in the same directory as the code being tested in "-test.cpp".
Unit tests placed in seperate directory force a lot of jumping around directory structure.

#### Enums

Avoid `default:` case in `switch` for enum type to enable compiler warning
when an enum option is missing.

Enum variables are effectively just ints and may hold value that is not matching
any declared enum option. Always handle these unknown values
(Lazy Input Validation).

````c++
    Enum Vehicle { CAR, TRAIN };

    std::string vehicleName(Vehicle x) {
        switch (x) {
            case Vehicle::CAR:
                return "car";
            case Vehicle::TRAIN: {
                return "train";
            }
        }
        return ds::str("unkown(", static_cast<int>(x), ")");
    }
````

## Formating rules

## Naming

* Type names: `TitleCase`
* Variable, member, function, and method names: `camelCase`
* Constant names: `CAPITAL_WITH_UNDERSCORES`
* Enumerant names: `CAPITAL_WITH_UNDERSCORES`
* Files: module-name.cpp, module-name.h, module-name-test.cpp
* Libraries: ds, ds-sqlite3
* Folders: prefer single words, follow rules for files
* Member class variables are prefixed with `m_`
* Member struct  are prefixed with `m_` TODO(soon): OR NOT?
* Static globally visible variables are prefixed with `g_` (but avoided as much as possible)
* Static member and static local variables are prefixed with `s_`
* Setters start with `set`, getters start with `get`.
    Use `get` prefix even for `bool` type properties to keep
    consistency with code generated from capnp IDL.
* Avoid abbreviation, do not delete letters within word
* Capitalize acronyms as single words. `startRpc()`, `class Rpc`
* Trivial functions should be inlined. Constructors and destructors for non - POD types are usually NOT trivial.

````c++
    class MyClass : public OtherClass {
    public:
        MyClass() = default;
        explicit MyClass(int number) : m_number(number) {}

        void someFunction();

        bool getIsOptional() { return m_isOptional; }
        void setIsOptional(bool x) { m_isOptional = x; }

        int getNumber() { return m_number; }
        void setNumber(int x) { m_number = x; }

        std::string getString() { return m_string; }
        void setString(const std::string& x) { m_string = x; }
        void setString(std::string&& x) { m_string = std::move(x); }

    private:
        bool m_isOtional = 0;
        int m_number = 0;
        std::string m_string;
    }
````

### Spacing and bracing

Do not waste time fixing and reviewing white space. Use `clang-format`.

TODO(soon): As `clang-format` produces different output on different versions,
we need to align on one version.

* Indents are 4 spaces
* Never use tabs
* Maximum line length is 120 characters
* Indent a continuation line 2x indent = 8 spaces
* Place a space between a keyword and an open parenthesis, e.g.: `if (foo)`
* Do not place a space between a function name and an open parenthesis, e.g.: `foo(bar)`
* Place an opening brace at the end of the statement which initiates the block,
    not on its own line.
* Place a closing brace on a new line indented the same as the parent block.
    If there is post-brace code related to the block (e.g. `else` or `while`),
    place it on the same line as the closing brace.

````c++
    if (s == "a") {
        return "A";
    } else if (s == "B") {
        return "B";
    } else {
        return "";
    }
````

* Always place braces around a block.

````c++
    if (done) {
        return;
    }
````

* `case` statements are indented within the switch, and their following
    blocks are _further_ indented (so the actual statements in a case
    are 2x more indented than the `switch`).

````c++
    switch (x) {
        case 0:
            return "a";
        case 1: {
            std::string s = "a";
            return s + s;
        }
        default: {
            std::string s = "b";
            return b + b;
        }
    }
````

* `public:`, `private:`, and `protected:` are reverse-indented by one stop.
* Set your editor to strip trailing whitespace on save, otherwise other people
    who use this setting will see spurious diffs when they edit a file after you.

### Comments

Always use line comments (`//`, `///`, `///<`). Never use block comments (`/**/`).
Use one space after `//`. Use one space before `//` or align with comments around.

````cpp
    return true; // Success
````

TODO comments are of the form `// TODO(type): description`, where `type` is one of:
* now: Do before next git push.
* soon: Do before next release.
* someday: A feature that might be nice to have some day, but no urgency.
* perf: Possible performance enhancement.
* memory: Possible memory overhead enhancement.
* cleanup: An improvement to maintainability with no user-visible effects.
* test: Something that needs better testing.
* upstream: Code upstreamable to more generic place.
* others: Additional TODO types may be defined for use in certain contexts.
