#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_template_test_macros.hpp>

#include "include/utilities.hpp"
#include "include/lifetime.hpp"
#include "include/universe.hpp"

#include <varerr/status.hpp>

using namespace varerr::tests;
using namespace varerr::tests::lifetime;
using namespace varerr::tests::universe;

namespace {

} // namespace

TEST_CASE("varerr_status_stub", "[varerr][status]") {

    STATIC_REQUIRE(true);

}
