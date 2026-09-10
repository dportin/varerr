#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_template_test_macros.hpp>

#include "include/utilities.hpp"
#include "include/universe.hpp"
#include "varerr/utilities.hpp"

#include <varerr/algebra.hpp>
#include <varerr/status.hpp>

#include <cstddef>
#include <type_traits>
#include <utility>

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


template <typename S, typename E>
concept IsStatusGetWellFormed = requires {
    std::declval<S>().template get<E>();
};

TEST_CASE("varerr_status_constraints_get", "[varerr][status]") {

    constexpr std::size_t kTestIndexBound = 7;

    using R0 = varerr::Row<E<1>, E<3>, E<5>>;
    using B0 = varerr::status_from_row_t<UniverseE, R0>;

    // The error row must be non-empty.

    iterate_cvref_matrix<HomStatus<0>>([]<typename T>(const std::type_identity<T>) -> void {
        STATIC_REQUIRE_FALSE(IsStatusGetWellFormed<T, E<0>>);
        STATIC_REQUIRE_FALSE(IsStatusGetWellFormed<T, E<1>>);
    });

    // The element must be a member of the error row.

    iterate_index_sequence<kTestIndexBound>([]<std::size_t I>(const index_constant<I>) -> void {
        STATIC_REQUIRE(IsStatusGetWellFormed<B0&, E<I>> == varerr::row_elem_normalized_v<UniverseE, E<I>, R0>);
    });

    // Only non-volatile lvalue references are permitted.

    iterate_cvref_matrix<B0>([]<typename T>(const std::type_identity<T>) -> void {
        constexpr bool well_formed = std::is_lvalue_reference_v<T> && !std::is_volatile_v<std::remove_reference_t<T>>;
        STATIC_REQUIRE(IsStatusGetWellFormed<T, E<3>> == well_formed);
    });

    // The returned reference tracks the constness of the status object.

    iterate_const_matrix<B0>([]<typename T>(const std::type_identity<T>) -> void {
        STATIC_REQUIRE(std::same_as<
            decltype(std::declval<T&>().template get<E<3>>()),
            std::conditional_t<std::is_const_v<T>, const E<3>&, E<3>&>
        >);
    });

}

template <typename S, typename E>
concept IsStatusGetIfWellFormed = requires {
    std::declval<S>().template get_if<E>();
};

TEST_CASE("varerr_status_constraints_get_if", "[varerr][status]") {

    constexpr std::size_t kTestIndexBound = 7;

    using R0 = varerr::Row<E<1>, E<3>, E<5>>;
    using B0 = varerr::status_from_row_t<UniverseE, R0>;

    // The error row must be non-empty.

    iterate_cvref_matrix<HomStatus<0>>([]<typename T>(const std::type_identity<T>) -> void {
        STATIC_REQUIRE_FALSE(IsStatusGetIfWellFormed<T, E<0>>);
        STATIC_REQUIRE_FALSE(IsStatusGetIfWellFormed<T, E<1>>);
    });

    // The element must be a member of the error row.

    iterate_index_sequence<kTestIndexBound>([]<std::size_t I>(const index_constant<I>) -> void {
        STATIC_REQUIRE(IsStatusGetIfWellFormed<B0&, E<I>> == varerr::row_elem_normalized_v<UniverseE, E<I>, R0>);
    });

    // Only non-volatile lvalue references are permitted.

    iterate_cvref_matrix<B0>([]<typename T>(const std::type_identity<T>) -> void {
        constexpr bool well_formed = std::is_lvalue_reference_v<T> && !std::is_volatile_v<std::remove_reference_t<T>>;
        STATIC_REQUIRE(IsStatusGetIfWellFormed<T, E<3>> == well_formed);
    });

    // The returned pointer tracks the constness of the status object.

    iterate_const_matrix<B0>([]<typename T>(const std::type_identity<T>) -> void {
        STATIC_REQUIRE(std::same_as<
            decltype(std::declval<T&>().template get_if<E<3>>()),
            std::conditional_t<std::is_const_v<T>, const E<3>*, E<3>*>
        >);
    });

}

TEST_CASE("varerr_status_constraints_visit", "[varerr][status]") {

    REQUIRE(false);

}
