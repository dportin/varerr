#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_template_test_macros.hpp>

#include "include/utilities.hpp"
#include "include/lifetime.hpp"
#include "include/universe.hpp"

#include <varerr/utilities.hpp>
#include <varerr/storage.hpp>
#include <varerr/algebra.hpp>
#include <varerr/status.hpp>
#include <varerr/result.hpp>

#include <concepts>
#include <cstddef>
#include <type_traits>
#include <utility>

using namespace varerr::tests;
using namespace varerr::tests::universe;

namespace {

} // namespace

TEST_CASE("varerr_result_trivial", "[varerr][result]") {
    REQUIRE(false);
}

TEST_CASE("varerr_result_construct_emplace_value", "[varerr][result]") {
    REQUIRE(false);
}

TEST_CASE("varerr_result_construct_emplace_error", "[varerr][result]") {
    REQUIRE(false);
}

TEST_CASE("varerr_result_constraints_emplace_value", "[varerr][result]") {
    REQUIRE(false);
}

TEST_CASE("varerr_result_constraints_emplace_error", "[varerr][result]") {
    REQUIRE(false);
}

TEST_CASE("varerr_result_noexcept_emplace_value", "[varerr][result]") {
    REQUIRE(false);
}

TEST_CASE("varerr_result_noexcept_emplace_error", "[varerr][result]") {
    REQUIRE(false);
}

TEST_CASE("varerr_result_functional_emplace_value", "[varerr][result]") {
    REQUIRE(false);
}

TEST_CASE("varerr_result_functional_emplace_error", "[varerr][result]") {
    REQUIRE(false);
}
