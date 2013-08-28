#define BOOST_TEST_NO_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>
#include <boost/shared_ptr.hpp>
#include <iostream>

#define HAVE_CURL 1
#include "src/url.h"
#include "src/propertysystem.h"
#include "src/dss.h"
#include "src/event.h"

#include "webservice_replies.h"

using namespace dss;

BOOST_AUTO_TEST_SUITE(CloudTest)

BOOST_AUTO_TEST_CASE(cloudTest) {

    /* test sample response */
    std::cout << "Test cloud replies parser... ";
    try {
        const char *buf =  "{\"ReturnCode\":2,\"ReturnMessage\":\"The dSS was not found in the database\"}";
        ModelChangeResponse resp = parseModelChange(buf);
    } catch (ParseError &ex) {
        //ModelChangeResponse resp = { 99, "unkown error" };
        std::cout << "failed\n";
        return;
    }
    std::cout << "success\n";


    /* cloud api, TODO evtl. split into separate cloud unit test */
    URLResult result;

    try {

    boost::shared_ptr<URL> curl(new URL());
    std::cout << "Testing cloud ApartmentHasChanged event... ";
    PropertySystem propSystem;
    std::string aprtmntChange(propSystem.getStringValue(ModelChangedEvent::propPathUrl));
    aprtmntChange += "?apartmentChangeType=Apartment";
    aprtmntChange += "&dssid=3504175feff28d2044084179";

    int code = curl->request(aprtmntChange, POST, URL::emptyHeader, URL::emptyForm, &result);

    if (code != 200) {
        std::cout << "invalid return code " << std::dec << code << "\n";
        return;
    }

    ModelChangeResponse resp = parseModelChange(result.content());

    std::cout << "success\n";
    std::cout << "   code: " << std::dec << resp.code << " desc: " << resp.desc << "\n";

    } catch (ParseError &ex) {
        std::cout << "Invalid reply " << result.content();
        return;
    }
}

BOOST_AUTO_TEST_SUITE_END()
