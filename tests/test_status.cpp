#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_template_test_macros.hpp>

#include "include/utilities.hpp"
#include "include/universe.hpp"

#include <type_traits>
#include <varerr/status.hpp>

using namespace varerr::tests;
using namespace varerr::tests::universe;

namespace {

// Lift the homogeneous test universe to Status.

template <std::size_t N>
using HomStatus = pack_apply_t<
    bind_front_adapter<varerr::BasicStatus, UniverseE>,
    lift_index_sequence_t<E, N>
>;

static_assert(std::same_as<HomStatus<0>, varerr::Status<UniverseE>>);
static_assert(std::same_as<HomStatus<1>, varerr::Status<UniverseE, E<0>>>);
static_assert(std::same_as<HomStatus<2>, varerr::Status<UniverseE, E<0>, E<1>>>);

// Lift the heterogeneous test universe to Status for a fixed log-alignment.

template <std::size_t N, std::size_t A>
using HetStatus = pack_apply_t<
    bind_front_adapter<varerr::BasicStatus, UniverseH>,
    lift_index_sequence_t<index_bind_back_adapter<H, A>::template apply, N>
>;

static_assert(std::same_as<HetStatus<0, 3>, varerr::Status<UniverseH>>);
static_assert(std::same_as<HetStatus<1, 3>, varerr::Status<UniverseH, H<0, 3>>>);
static_assert(std::same_as<HetStatus<2, 3>, varerr::Status<UniverseH, H<0, 3>, H<1, 3>>>);

} // namespace

TEMPLATE_TEST_CASE("varerr_status_trivial", "[varerr][status]",
    HomStatus<0>, HomStatus<3>, (HetStatus<0, 3>), (HetStatus<3, 3>)
) {

    using TrivialStatus = varerr::Status<UniverseI, TrivialType>;

    // BasicStatus<M, Es...> inherits triviality from Storage.

    STATIC_REQUIRE(std::is_trivially_copyable_v<TestType>);
    STATIC_REQUIRE(std::is_trivially_destructible_v<TestType>);
    STATIC_REQUIRE(std::is_trivially_copy_constructible_v<TestType>);
    STATIC_REQUIRE(std::is_trivially_move_constructible_v<TestType>);
    STATIC_REQUIRE(std::is_trivially_copy_assignable_v<TestType>);
    STATIC_REQUIRE(std::is_trivially_move_assignable_v<TestType>);

    // BasicStatus<M, Es...> is never trivially default constructible.

    STATIC_REQUIRE_FALSE(std::is_trivially_default_constructible_v<HomStatus<1>>);
    STATIC_REQUIRE_FALSE(std::is_trivially_default_constructible_v<TrivialStatus>);

}

TEST_CASE("varerr_status_construct_default", "[varerr][status]") {

    using NoDefaultStatus = varerr::Status<UniverseI, NoDefaultConstructType>;

    // BasicStatus<M> is never constructible.

    STATIC_REQUIRE_FALSE(std::is_default_constructible_v<HomStatus<0>>);
    STATIC_REQUIRE_FALSE(std::is_default_constructible_v<HetStatus<0, 3>>);

    // BasicStatus<M, Es...> inherits default constructibility from Es...

    STATIC_REQUIRE(std::is_default_constructible_v<HomStatus<1>>);
    STATIC_REQUIRE_FALSE(std::is_default_constructible_v<NoDefaultStatus>);

}

TEST_CASE("varerr_status_construct", "[varerr][status]") {
    REQUIRE(false);
}

TEST_CASE("varerr_status_construct_emplace", "[varerr][status]") {
    REQUIRE(false);
}

TEST_CASE("varerr_status_construct_widen", "[varerr][status]") {
    REQUIRE(false);
}
