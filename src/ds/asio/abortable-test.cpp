#include "abortable.h"
#include <ds/asio/catch.h>

static const char* TAGS = "[dsAsioAbortable][dsAsio][ds]";

TEST_CASE("dsAsioAbortable", TAGS) {
    SECTION("can be constructed and destroyed") { ds::asio::Abortable abortable; }

    SECTION("handle is initially not aborted") {
        ds::asio::Abortable abortable;
        auto handle = abortable.nextHandle();
        REQUIRE(handle.isAborted() == false);
    }

    SECTION("handle is aborted on abort") {
        ds::asio::Abortable abortable;
        auto handle = abortable.nextHandle();
        abortable.abort();
        REQUIRE(handle.isAborted() == true);
    }

    SECTION("handle is aborted on nextHandle()") {
        ds::asio::Abortable abortable;
        auto handle = abortable.nextHandle();
        auto handle2 = abortable.nextHandle();
        REQUIRE(handle.isAborted() == true);
        REQUIRE(handle2.isAborted() == false);
    }

    SECTION("handle is aborted on abortable destroy()") {
        std::unique_ptr<ds::asio::Abortable> abortable(new ds::asio::Abortable());
        auto handle = abortable->nextHandle();
        abortable.reset();
        REQUIRE(handle.isAborted() == true);
    }
}
