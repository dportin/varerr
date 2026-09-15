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
#include <expected>
#include <type_traits>
#include <utility>

using namespace varerr::tests;
using namespace varerr::tests::universe;

namespace {

// Lift the homogeneous test universe to BasicStatus.

template <std::size_t N>
using HomStatus = pack_apply_t<
    bind_front_adapter<varerr::BasicStatus, UniverseE>,
    lift_index_sequence_t<E, N>
>;

static_assert(std::same_as<HomStatus<0>, varerr::Status<UniverseE>>);
static_assert(std::same_as<HomStatus<1>, varerr::Status<UniverseE, E<0>>>);
static_assert(std::same_as<HomStatus<2>, varerr::Status<UniverseE, E<0>, E<1>>>);

// Lift the homogeneous test universe to BasicResult.

template <typename T, std::size_t N>
using HomResult = pack_apply_t<
    bind_front_adapter<varerr::BasicResult, UniverseE, T>,
    lift_index_sequence_t<E, N>
>;

static_assert(std::same_as<HomResult<int, 0>, varerr::Result<UniverseE, int>>);
static_assert(std::same_as<HomResult<int, 1>, varerr::Result<UniverseE, int, E<0>>>);
static_assert(std::same_as<HomResult<int, 2>, varerr::Result<UniverseE, int, E<0>, E<1>>>);

// Lift the heterogeneous test universe to BasicStatus.

template <std::size_t N, std::size_t A>
using HetStatus = pack_apply_t<
    bind_front_adapter<varerr::BasicStatus, UniverseH>,
    lift_index_sequence_t<index_bind_back_adapter<H, A>::template apply, N>
>;

static_assert(std::same_as<HetStatus<0, 3>, varerr::Status<UniverseH>>);
static_assert(std::same_as<HetStatus<1, 3>, varerr::Status<UniverseH, H<0, 3>>>);
static_assert(std::same_as<HetStatus<2, 3>, varerr::Status<UniverseH, H<0, 3>, H<1, 3>>>);

// Lift the heterogeneous test universe to BasicResult.

template <typename T, std::size_t N, std::size_t A>
using HetResult = pack_apply_t<
    bind_front_adapter<varerr::BasicResult, UniverseH, T>,
    lift_index_sequence_t<index_bind_back_adapter<H, A>::template apply, N>
>;

static_assert(std::same_as<HetResult<int, 0, 3>, varerr::Result<UniverseH, int>>);
static_assert(std::same_as<HetResult<int, 1, 3>, varerr::Result<UniverseH, int, H<0, 3>>>);
static_assert(std::same_as<HetResult<int, 2, 3>, varerr::Result<UniverseH, int, H<0, 3>, H<1, 3>>>);

// Determine whether a non-void BasicResult is copy or move assignable.

template <typename T>
inline constexpr bool is_copy_assignable_depends_v =
    std::is_copy_assignable_v<T> &&
    std::is_copy_constructible_v<T>;

template <typename T>
inline constexpr bool is_move_assignable_depends_v =
    std::is_move_assignable_v<T> &&
    std::is_move_constructible_v<T>;

// Determine whether a non-void BasicResult is trivially copy or move assignable.

template <typename T>
inline constexpr bool is_trivially_copy_assignable_depends_v =
    std::is_trivially_copy_assignable_v<T> &&
    std::is_trivially_copy_constructible_v<T>;

template <typename T>
inline constexpr bool is_trivially_move_assignable_depends_v =
    std::is_trivially_move_assignable_v<T> &&
    std::is_trivially_move_constructible_v<T>;

} // namespace

TEMPLATE_TEST_CASE("varerr_result_trivial", "[varerr][result]",
    TrivialType,
    NonTrivialConstructType,
    NonTrivialDefaultConstructType,
    NonTrivialCopyConstructType,
    NonTrivialMoveConstructType,
    NonTrivialCopyAssignType,
    NonTrivialMoveAssignType,
    NonTrivialDestructType,
    NonStandardLayoutType
) {

    iterate_index_array<0, 1>([]<std::size_t I>(const index_constant<I>) -> void {

        using ValueType = TestType;
        using ResultType = HomResult<ValueType, I>;

        // Ensure triviality assertions are not vacuous.

        STATIC_REQUIRE(std::is_destructible_v<ResultType> == std::is_destructible_v<ValueType>);
        STATIC_REQUIRE(std::is_copy_constructible_v<ResultType> == std::is_copy_constructible_v<ValueType>);
        STATIC_REQUIRE(std::is_move_constructible_v<ResultType> == std::is_move_constructible_v<ValueType>);
        STATIC_REQUIRE(std::is_copy_assignable_v<ResultType> == is_copy_assignable_depends_v<ValueType>);
        STATIC_REQUIRE(std::is_move_assignable_v<ResultType> == is_move_assignable_depends_v<ValueType>);
        STATIC_REQUIRE(std::is_default_constructible_v<ResultType> == std::is_default_constructible_v<ValueType>);

        // A non-void BasicResult inherits triviality from the value type.

        STATIC_REQUIRE(std::is_trivially_copyable_v<ResultType> == std::is_trivially_copyable_v<ValueType>);
        STATIC_REQUIRE(std::is_trivially_destructible_v<ResultType> == std::is_trivially_destructible_v<ValueType>);
        STATIC_REQUIRE(std::is_trivially_copy_constructible_v<ResultType> == std::is_trivially_copy_constructible_v<ValueType>);
        STATIC_REQUIRE(std::is_trivially_move_constructible_v<ResultType> == std::is_trivially_move_constructible_v<ValueType>);
        STATIC_REQUIRE(std::is_trivially_copy_assignable_v<ResultType> == is_trivially_copy_assignable_depends_v<ValueType>);
        STATIC_REQUIRE(std::is_trivially_move_assignable_v<ResultType> == is_trivially_move_assignable_depends_v<ValueType>);

        // A non-void BasicResult is never trivially default constructible.

        STATIC_REQUIRE_FALSE(std::is_trivially_default_constructible_v<ResultType>);

    });

}

TEST_CASE("varerr_result_trivial_void", "[varerr][result]") {

    iterate_index_array<0, 1>([]<std::size_t I>(const index_constant<I>) -> void {

        using ValueType = void;
        using ResultType = HomResult<ValueType, I>;

        // Ensure triviality assertions are not vacuous.

        STATIC_REQUIRE(std::is_destructible_v<ResultType>);
        STATIC_REQUIRE(std::is_copy_constructible_v<ResultType>);
        STATIC_REQUIRE(std::is_move_constructible_v<ResultType>);
        STATIC_REQUIRE(std::is_copy_assignable_v<ResultType>);
        STATIC_REQUIRE(std::is_move_assignable_v<ResultType>);
        STATIC_REQUIRE(std::is_default_constructible_v<ResultType>);

        // A void BasicResult is trivial.

        STATIC_REQUIRE(std::is_trivially_copyable_v<ResultType>);
        STATIC_REQUIRE(std::is_trivially_destructible_v<ResultType>);
        STATIC_REQUIRE(std::is_trivially_copy_constructible_v<ResultType>);
        STATIC_REQUIRE(std::is_trivially_move_constructible_v<ResultType>);
        STATIC_REQUIRE(std::is_trivially_copy_assignable_v<ResultType>);
        STATIC_REQUIRE(std::is_trivially_move_assignable_v<ResultType>);

        // A void BasicResult is never trivially default constructible.

        STATIC_REQUIRE_FALSE(std::is_trivially_default_constructible_v<ResultType>);

    });

}

TEST_CASE("varerr_result_construct_empty", "[varerr][result]") {
    REQUIRE(false);
}

TEST_CASE("varerr_result_construct_default", "[varerr][result]") {
    REQUIRE(false);
}

TEST_CASE("varerr_result_construct_emplace_value", "[varerr][result]") {
    REQUIRE(false);
}

TEST_CASE("varerr_result_construct_emplace_error", "[varerr][result]") {
    REQUIRE(false);
}

TEST_CASE("varerr_result_construct_widen", "[varerr][result]") {
    REQUIRE(false);
}

TEST_CASE("varerr_result_constraints_emplace_value", "[varerr][result]") {
    REQUIRE(false);
}

TEST_CASE("varerr_result_constraints_emplace_error", "[varerr][result]") {
    REQUIRE(false);
}

TEST_CASE("varerr_result_constraints_widen", "[varerr][result]") {
    REQUIRE(false);
}

TEST_CASE("varerr_result_noexcept_emplace_value", "[varerr][result]") {
    REQUIRE(false);
}

TEST_CASE("varerr_result_noexcept_emplace_error", "[varerr][result]") {
    REQUIRE(false);
}

TEST_CASE("varerr_result_noexcept_emplace_widen", "[varerr][result]") {
    REQUIRE(false);
}

TEST_CASE("varerr_result_functional_emplace", "[varerr][result]") {
    REQUIRE(false);
}

TEST_CASE("varerr_result_functional_widen", "[varerr][result]") {
    REQUIRE(false);
}
