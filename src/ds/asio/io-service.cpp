#include "io-service.h"
#include <ds/log.h>

namespace ds {
namespace asio {


IoService::IoService() : boost::asio::io_service(1), m_threadId(std::this_thread::get_id()) { }
IoService::~IoService() = default;

std::size_t IoService::run() {
  DS_REQUIRE(thisThreadMatches(), "run() is called from wrong thread");
  return boost::asio::io_service::run();
}

} // namespace asio
} // namespace ds
