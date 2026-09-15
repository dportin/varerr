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

template <std::size_t N, std::size_t Offset = 0, std::size_t Stride = 1>
using HomStatus = pack_apply_t<
    bind_front_adapter<varerr::BasicStatus, UniverseE>,
    lift_index_sequence_t<E, N, Offset, Stride>
>;

static_assert(std::same_as<HomStatus<0>, varerr::Status<UniverseE>>);
static_assert(std::same_as<HomStatus<1, 1>, varerr::Status<UniverseE, E<1>>>);
static_assert(std::same_as<HomStatus<3, 1, 2>, varerr::Status<UniverseE, E<1>, E<3>, E<5>>>);

// Lift the homogeneous test universe to BasicResult.

template <typename T, std::size_t N, std::size_t Offset = 0, std::size_t Stride = 1>
using HomResult = pack_apply_t<
    bind_front_adapter<varerr::BasicResult, UniverseE, T>,
    lift_index_sequence_t<E, N, Offset, Stride>
>;

static_assert(std::same_as<HomResult<int, 0>, varerr::Result<UniverseE, int>>);
static_assert(std::same_as<HomResult<int, 1, 1>, varerr::Result<UniverseE, int, E<1>>>);
static_assert(std::same_as<HomResult<int, 3, 1, 2>, varerr::Result<UniverseE, int, E<1>, E<3>, E<5>>>);

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

// Determine whether BasicResult constructors and member functions are well-formed.

template <typename R>
concept IsResultHasValueWellFormed = requires {
    std::declval<R>().has_value();
};

// template <typename R>
// concept IsResultBoolWellFormed = std::constructible_from<bool, R>;

// template <typename R>
// concept IsResultImplicitBoolWellFormed = std::convertible_to<R, bool>;

template <typename R>
concept IsResultHasErrorWellFormed = requires {
    std::declval<R>().has_error();
};

template <typename R, typename E>
concept IsResultHoldsErrorWellFormed = requires {
    std::declval<R>().template holds_error<E>();
};

template <typename R>
concept IsResultValueWellFormed = requires {
    std::declval<R>().value();
};

template <typename R>
concept IsResultValueIfWellFormed = requires {
    std::declval<R>().value_if();
};

template <typename R, typename E>
concept IsResultErrorIfWellFormed = requires {
    std::declval<R>().template error_if<E>();
};

template <typename R, typename E>
concept IsResultErrorWellFormed = requires {
    std::declval<R>().template error<E>();
};

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
    (HomResult<TrivialType, 0>),
    (HomResult<TrivialType, 3, 1, 2>),
    (HomResult<NonTrivialConstructType, 0>),
    (HomResult<NonTrivialConstructType, 3, 1, 2>),
    (HomResult<NonTrivialDefaultConstructType, 0>),
    (HomResult<NonTrivialDefaultConstructType, 3, 1, 2>),
    (HomResult<NonTrivialCopyConstructType, 0>),
    (HomResult<NonTrivialCopyConstructType, 3, 1, 2>),
    (HomResult<NonTrivialMoveConstructType, 0>),
    (HomResult<NonTrivialMoveConstructType, 3, 1, 2>),
    (HomResult<NonTrivialCopyAssignType, 0>),
    (HomResult<NonTrivialCopyAssignType, 3, 1, 2>),
    (HomResult<NonTrivialMoveAssignType, 0>),
    (HomResult<NonTrivialMoveAssignType, 3, 1, 2>),
    (HomResult<NonTrivialDestructType, 0>),
    (HomResult<NonTrivialDestructType, 3, 1, 2>),
    (HomResult<NonStandardLayoutType, 0>),
    (HomResult<NonStandardLayoutType, 3, 1, 2>)
) {

    using ResultType = TestType;
    using ValueType = varerr::result_value_t<ResultType>;

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

}

TEMPLATE_TEST_CASE("varerr_result_trivial_void", "[varerr][result]",
    (HomResult<void, 0>),
    (HomResult<void, 3, 1, 2>)
) {

    using ResultType = TestType;

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

TEMPLATE_TEST_CASE("varerr_result_constraints_has_value", "[varerr][result]",
    (HomResult<void, 0>),
    (HomResult<void, 3, 1, 2>),
    (HomResult<TrivialType, 0>),
    (HomResult<TrivialType, 3, 1, 2>)
) {

    using ResultType = TestType;

    iterate_cvref_matrix<ResultType>([]<typename T>(const std::type_identity<T>) -> void {
        constexpr bool is_volatile = std::is_volatile_v<std::remove_reference_t<T>>;
        STATIC_REQUIRE(IsResultHasValueWellFormed<T> == !is_volatile);
    });

}

TEMPLATE_TEST_CASE("varerr_result_contraints_has_error", "[varerr][result]",
    (HomResult<void, 0>),
    (HomResult<void, 3, 1, 2>),
    (HomResult<TrivialType, 0>),
    (HomResult<TrivialType, 3, 1, 2>)
) {

    using ResultType = TestType;
    constexpr std::size_t kTestIndexBound = 7;

    iterate_cvref_matrix<ResultType>([]<typename T>(const std::type_identity<T>) -> void {
        iterate_index_sequence<kTestIndexBound>([]<std::size_t I>(const index_constant<I>) -> void {
            constexpr bool is_volatile = std::is_volatile_v<std::remove_reference_t<T>>;
            STATIC_REQUIRE(IsResultHasErrorWellFormed<T> == !is_volatile);
        });
    });

}


TEMPLATE_TEST_CASE("varerr_result_constraints_holds_error", "[varerr][result]",
    (HomResult<void, 0>),
    (HomResult<void, 3, 1, 2>),
    (HomResult<TrivialType, 0>),
    (HomResult<TrivialType, 3, 1, 2>)
) {

    using ResultType = TestType;
    constexpr std::size_t kTestIndexBound = 7;

    iterate_cvref_matrix<ResultType>([]<typename T>(const std::type_identity<T>) -> void {
        iterate_index_sequence<kTestIndexBound>([]<std::size_t I>(const index_constant<I>) -> void {
            constexpr bool is_volatile = std::is_volatile_v<std::remove_reference_t<T>>;
            constexpr bool is_alternative = varerr::IsElemExactInRow<UniverseE, varerr::result_row_t<T>, E<I>>;
            STATIC_REQUIRE(IsResultHoldsErrorWellFormed<T, E<I>> == (!is_volatile && is_alternative));
        });
    });

}

TEMPLATE_TEST_CASE("varerr_result_constraints_value_if", "[varerr][result]",
    (HomResult<void, 0>),
    (HomResult<void, 3, 1, 2>),
    (HomResult<TrivialType, 0>),
    (HomResult<TrivialType, 3, 1, 2>)
) {


    using ResultType = TestType;
    using ValueType = varerr::result_value_t<ResultType>;

    iterate_cvref_matrix<ResultType>([]<typename T>(const std::type_identity<T>) -> void {
        constexpr bool is_void = std::is_void_v<ValueType>;
        constexpr bool is_volatile = std::is_volatile_v<std::remove_reference_t<T>>;
        constexpr bool is_lvalue_ref = std::is_lvalue_reference_v<T>;
        STATIC_REQUIRE(IsResultValueIfWellFormed<T> == (!is_void && !is_volatile && is_lvalue_ref));
    });

}

TEMPLATE_TEST_CASE("varerr_result_constraints_value", "[varerr][result]",
    (HomResult<void, 0>),
    (HomResult<void, 3, 1, 2>),
    (HomResult<TrivialType, 0>),
    (HomResult<TrivialType, 3, 1, 2>)
) {

    using ResultType = TestType;
    using ValueType = varerr::result_value_t<ResultType>;

    iterate_cvref_matrix<ResultType>([]<typename T>(const std::type_identity<T>) -> void {
        constexpr bool is_void = std::is_void_v<ValueType>;
        constexpr bool is_volatile = std::is_volatile_v<std::remove_reference_t<T>>;
        STATIC_REQUIRE(IsResultValueWellFormed<T> == (!is_void && !is_volatile));
    });

}

TEMPLATE_TEST_CASE("varerr_result_constraints_error_if", "[varerr][result]",
    (HomResult<void, 0>),
    (HomResult<void, 3, 1, 2>),
    (HomResult<TrivialType, 0>),
    (HomResult<TrivialType, 3, 1, 2>)
) {

    using ResultType = TestType;
    constexpr std::size_t kTestIndexBound = 7;

    iterate_cvref_matrix<ResultType>([]<typename T>(const std::type_identity<T>) -> void {
        iterate_index_sequence<kTestIndexBound>([]<std::size_t I>(const index_constant<I>) -> void {
            constexpr bool is_volatile = std::is_volatile_v<std::remove_reference_t<T>>;
            constexpr bool is_lvalue_ref = std::is_lvalue_reference_v<T>;
            constexpr bool is_alternative = varerr::IsElemExactInRow<UniverseE, varerr::result_row_t<T>, E<I>>;
            STATIC_REQUIRE(IsResultErrorIfWellFormed<T, E<I>> == (!is_volatile && is_lvalue_ref && is_alternative));
        });
    });

}

TEMPLATE_TEST_CASE("varerr_result_constraints_error", "[varerr][result]",
    (HomResult<void, 0>),
    (HomResult<void, 3, 1, 2>),
    (HomResult<TrivialType, 0>),
    (HomResult<TrivialType, 3, 1, 2>)
) {

    using ResultType = TestType;
    constexpr std::size_t kTestIndexBound = 7;

    iterate_cvref_matrix<ResultType>([]<typename T>(const std::type_identity<T>) -> void {
        iterate_index_sequence<kTestIndexBound>([]<std::size_t I>(const index_constant<I>) -> void {
            constexpr bool is_volatile = std::is_volatile_v<std::remove_reference_t<T>>;
            constexpr bool is_lvalue_ref = std::is_lvalue_reference_v<T>;
            constexpr bool is_alternative = varerr::IsElemExactInRow<UniverseE, varerr::result_row_t<T>, E<I>>;
            STATIC_REQUIRE(IsResultErrorWellFormed<T, E<I>> == (!is_volatile && is_lvalue_ref && is_alternative));
        });
    });

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

TEST_CASE("varerr_result_noexcept_has_value", "[varerr][result]") {
    REQUIRE(false);
}

TEST_CASE("varerr_result_noexcept_has_error", "[varerr][result]") {
    REQUIRE(false);
}

TEST_CASE("varerr_result_noexcept_holds_error", "[varerr][result]") {
    REQUIRE(false);
}

TEST_CASE("varerr_result_noexcept_value_if", "[varerr][result]") {
    REQUIRE(false);
}

TEST_CASE("varerr_result_noexcept_value", "[varerr][result]") {
    REQUIRE(false);
}

TEST_CASE("varerr_result_return_value_if", "[varerr][result]") {
    REQUIRE(false);
}

TEST_CASE("varerr_result_return_value", "[varerr][result]") {
    REQUIRE(false);
}

TEST_CASE("varerr_result_functional_emplace", "[varerr][result]") {
    REQUIRE(false);
}

TEST_CASE("varerr_result_functional_widen", "[varerr][result]") {
    REQUIRE(false);
}

TEST_CASE("varerr_result_functional_has_value", "[varerr][result]") {
    REQUIRE(false);
}

TEST_CASE("varerr_result_functional_has_error", "[varerr][result]") {
    REQUIRE(false);
}

TEST_CASE("varerr_result_functional_holds_error", "[varerr][result]") {
    REQUIRE(false);
}

TEST_CASE("varerr_result_functional_holds_value_if", "[varerr][result]") {
    REQUIRE(false);
}

TEST_CASE("varerr_result_functional_holds_value", "[varerr][result]") {
    REQUIRE(false);
}
