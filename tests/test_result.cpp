#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_template_test_macros.hpp>

#include "include/utilities.hpp"
// #include "include/lifetime.hpp"
#include "include/universe.hpp"
#include "include/functor.hpp"

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
using namespace varerr::tests::functor;

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

// Lift the invariant-breaking test universe to BasicResult.

template <typename T, typename... Es>
using InvResult = pack_apply_t<
    bind_front_adapter<varerr::BasicResult, pack_apply_t<bind_adapter<UniverseT>, varerr::Row<Es...>>, T>,
    varerr::Row<Es...>
>;

static_assert(std::same_as<InvResult<int>, varerr::Result<UniverseT<>, int>>);
static_assert(std::same_as<InvResult<int, short>, varerr::Result<UniverseT<short>, int, short>>);
static_assert(std::same_as<InvResult<int, short, E<0>>, varerr::Result<UniverseT<short, E<0>>, int, short, E<0>>>);

template <typename R>
concept IsResultHasValueWellFormed = requires {
    std::declval<R>().has_value();
};

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

template <typename R>
concept IsResultTakeWellFormed = requires {
    std::declval<R>().take();
};

template <typename R, typename E>
concept IsResultErrorIfWellFormed = requires {
    std::declval<R>().template error_if<E>();
};

template <typename R, typename E>
concept IsResultErrorWellFormed = requires {
    std::declval<R>().template error<E>();
};

template <typename R, typename F>
concept IsResultTransformWellFormed = requires (R r, F f) {
    std::forward<R>(r).transform(std::forward<F>(f));
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

// Aliases for testing widening constructor.

template <typename T>
using ResultVarEmptyType = HomResult<T, 0>;

template <typename T>
using ResultVarSubType = HomResult<T, 3, 3, 2>;

template <typename T>
using ResultVarSuperType = HomResult<T, 5, 1, 2>;

template <typename T>
using ResultVarLeftType = HomResult<T, 3, 1, 2>;

template <typename T>
using ResultVarRightType = HomResult<T, 3, 5, 2>;

static_assert(std::same_as<ResultVarEmptyType<int>, varerr::BasicResult<UniverseE, int>>);
static_assert(std::same_as<ResultVarSubType<int>, varerr::BasicResult<UniverseE, int, E<3>, E<5>, E<7>>>);
static_assert(std::same_as<ResultVarSuperType<int>, varerr::BasicResult<UniverseE, int, E<1>, E<3>, E<5>, E<7>, E<9>>>);
static_assert(std::same_as<ResultVarLeftType<int>, varerr::BasicResult<UniverseE, int, E<1>, E<3>, E<5>>>);
static_assert(std::same_as<ResultVarRightType<int>, varerr::BasicResult<UniverseE, int, E<5>, E<7>, E<9>>>);

} // namespace

// Triviality tests.

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

// Constructibility tests.

TEST_CASE("varerr_result_construct_empty", "[varerr][result]") {

    using ResultEmptyType = HomResult<TrivialType, 0>;
    using ResultEmptyVoidType = HomResult<void, 0>;

    // An empty BasicResult has no in-place error constructor.

    STATIC_REQUIRE_FALSE(std::is_constructible_v<ResultEmptyType, std::in_place_type_t<E<0>>, std::size_t>);
    STATIC_REQUIRE_FALSE(std::is_constructible_v<ResultEmptyVoidType, std::in_place_type_t<E<0>>, std::size_t>);

    // An empty BasicResult holds a value.

    constexpr ResultEmptyType result_empty_value { std::in_place, 42 };

    STATIC_REQUIRE(result_empty_value.has_value());
    STATIC_REQUIRE(result_empty_value.value().value_ == 42);
    STATIC_REQUIRE_FALSE(result_empty_value.has_error());

    constexpr ResultEmptyVoidType result_void { std::in_place };

    STATIC_REQUIRE(result_void.has_value());
    STATIC_REQUIRE_FALSE(result_void.has_error());

}

TEST_CASE("varerr_result_construct_default", "[varerr][result]") {

    using ResultDefType = HomResult<TrivialType, 1>;
    using ResultDefVoidType = HomResult<void, 1>;
    using ResultDefThrowType = HomResult<ThrowDefaultConstructType, 1>;
    using ResultNotDefType = HomResult<NoDefaultConstructType, 1>;

    // The default constructor inherits default constructibility from the value
    // type.

    STATIC_REQUIRE(std::is_default_constructible_v<ResultDefType>);
    STATIC_REQUIRE(std::is_default_constructible_v<ResultDefVoidType>);
    STATIC_REQUIRE(std::is_default_constructible_v<ResultDefThrowType>);
    STATIC_REQUIRE_FALSE(std::is_default_constructible_v<ResultNotDefType>);

    // The default constructor value-initializes the value.

    constexpr ResultDefType result_def {};

    STATIC_REQUIRE(result_def.has_value());
    STATIC_REQUIRE(result_def.value().value_ == 0);
    STATIC_REQUIRE_FALSE(result_def.has_error());

    constexpr ResultDefVoidType result_def_void {};

    STATIC_REQUIRE(result_def_void.has_value());
    STATIC_REQUIRE_FALSE(result_def_void.has_error());

    constexpr ResultDefThrowType result_def_throw {};

    STATIC_REQUIRE(result_def_throw.has_value());
    STATIC_REQUIRE(result_def_throw.value().value_ == 0);
    STATIC_REQUIRE_FALSE(result_def_throw.has_error());

}

TEST_CASE("varerr_result_construct_emplace_value", "[varerr][result]") {

    using ValueUnaryType = TrivialType;
    using ValueBinaryType = std::pair<int, int>;

    using ResultUnaryType = HomResult<ValueUnaryType, 1>;
    using ResultBinaryType = HomResult<ValueBinaryType, 1>;

    // The in-place value constructor inherits constructibility from the value
    // type.

    STATIC_REQUIRE(std::is_constructible_v<ResultUnaryType, std::in_place_t, int>);
    STATIC_REQUIRE(std::is_constructible_v<ResultBinaryType, std::in_place_t, int, int>);

    // The in-place value constructor constructs the value branch.

    constexpr ResultUnaryType result_unary { std::in_place, 42 };

    STATIC_REQUIRE(result_unary.has_value());
    STATIC_REQUIRE(result_unary.value().value_ == 42);
    STATIC_REQUIRE_FALSE(result_unary.has_error());

    constexpr ResultBinaryType result_binary { std::in_place, 42, 43 };

    STATIC_REQUIRE(result_binary.has_value());
    STATIC_REQUIRE(result_binary.value().first == 42);
    STATIC_REQUIRE(result_binary.value().second == 43);
    STATIC_REQUIRE_FALSE(result_binary.has_error());

    // The in-place value constructor value-initializes the value when no argum-
    // ents are supplied.

    constexpr ResultUnaryType result_unary_noargs { std::in_place };

    STATIC_REQUIRE(result_unary_noargs.has_value());
    STATIC_REQUIRE(result_unary_noargs.value().value_ == 0);
    STATIC_REQUIRE_FALSE(result_unary_noargs.has_error());

    constexpr ResultBinaryType result_binary_noargs { std::in_place };

    STATIC_REQUIRE(result_binary_noargs.has_value());
    STATIC_REQUIRE(result_binary_noargs.value().first == 0);
    STATIC_REQUIRE(result_binary_noargs.value().second == 0);
    STATIC_REQUIRE_FALSE(result_binary_noargs.has_error());

}

TEST_CASE("varerr_result_construct_emplace_value_forward", "[varerr][result]") {

    using ValueForwardType = ForwardProbeType;
    using ResultForwardType = HomResult<ValueForwardType, 1>;

    // The in-place value constructor forwards its arguments.

    STATIC_REQUIRE([]() -> std::pair<ForwardCategory, ForwardCategory> {
        int fst = 42; const int snd = 43;
        ResultForwardType result { std::in_place, fst, std::move(snd) }; // NOLINT
        return { result.value().fst_, result.value().snd_ };
    }() == std::pair { ForwardCategory::LValue, ForwardCategory::ConstRValue });

    STATIC_REQUIRE([]() -> std::pair<ForwardCategory, ForwardCategory> {
        int fst = 42; const int snd = 43;
        ResultForwardType result { std::in_place, std::move(fst), snd }; // NOLINT
        return { result.value().fst_, result.value().snd_ };
    }() == std::pair { ForwardCategory::RValue, ForwardCategory::ConstLValue });

}

TEST_CASE("varerr_result_construct_emplace_error", "[varerr][result]") {

    constexpr std::size_t kTestIndexBound = 7;

    using ResultType = HomResult<TrivialType, 3, 1, 2>;
    using RowType = varerr::result_row_t<ResultType>;
    using UniverseType = varerr::result_universe_t<ResultType>;

    // The in-place error constructor inherits constructibility from the error
    // alternative.

    iterate_index_sequence<kTestIndexBound>([]<std::size_t I>(const index_constant<I>) -> void {
        constexpr bool is_alternative = varerr::IsElemExactInRow<UniverseType, RowType, E<I>>;
        STATIC_REQUIRE(std::is_constructible_v<ResultType, std::in_place_type_t<E<I>>, std::size_t> == is_alternative);
    });

    // The in-place error constructor constructs the error alternative by type
    // in the error branch.

    iterate_index_array<1, 3, 5>([]<std::size_t I>(const index_constant<I>) -> void {
        constexpr ResultType result { std::in_place_type<E<I>>, std::size_t {I + 42} };
        STATIC_REQUIRE(result.has_error());
        STATIC_REQUIRE(result.holds_error<E<I>>());
        STATIC_REQUIRE(result.error<E<I>>().value() == I + 42);
        STATIC_REQUIRE_FALSE(result.has_value());
    });

    // The in-place error constructor value-initializes the error alternative
    // when no arguments are supplied.

    iterate_index_array<1, 3, 5>([]<std::size_t I>(const index_constant<I>) -> void {
        constexpr ResultType result { std::in_place_type<E<I>> };
        STATIC_REQUIRE(result.has_error());
        STATIC_REQUIRE(result.holds_error<E<I>>());
        STATIC_REQUIRE(result.error<E<I>>().value() == 0);
        STATIC_REQUIRE_FALSE(result.has_value());
    });

}

TEST_CASE("varerr_result_construct_emplace_error_forward", "[varerr][result]") {

    using ResultForwardType = InvResult<TrivialType, E<0>, ForwardProbeType, E<1>>;

    // The in-place error constructor forwards its arguments.

    STATIC_REQUIRE([]() -> std::pair<ForwardCategory, ForwardCategory> {
        int fst = 42; const int snd = 43;
        ResultForwardType result { std::in_place_type<ForwardProbeType>, fst, std::move(snd) }; // NOLINT
        return { result.error<ForwardProbeType>().fst_, result.error<ForwardProbeType>().snd_ };
    }() == std::pair { ForwardCategory::LValue, ForwardCategory::ConstRValue });

    STATIC_REQUIRE([]() -> std::pair<ForwardCategory, ForwardCategory> {
        int fst = 42; const int snd = 43;
        ResultForwardType result { std::in_place_type<ForwardProbeType>, std::move(fst), snd }; // NOLINT
        return { result.error<ForwardProbeType>().fst_, result.error<ForwardProbeType>().snd_ };
    }() == std::pair { ForwardCategory::RValue, ForwardCategory::ConstLValue });

}

TEST_CASE("varerr_result_construct_widen", "[varerr][result]") {

    using RowEmpty = varerr::Row<>;
    using RowBase = varerr::Row<E<1>, E<3>>;
    using RowPrefixed = varerr::Row<E<1>, E<3>, E<5>>;
    using RowUnPrefixed = varerr::Row<E<0>, E<1>, E<2>, E<3>, E<4>>;

    using ResultEmpty = varerr::result_from_row_t<UniverseE, TrivialType, RowEmpty>;
    using ResultBase = varerr::result_from_row_t<UniverseE, TrivialType, RowBase>;
    using ResultPrefixed = varerr::result_from_row_t<UniverseE, TrivialType, RowPrefixed>;
    using ResultUnPrefixed = varerr::result_from_row_t<UniverseE, TrivialType, RowUnPrefixed>;

    // Widening propagates the active value.

    constexpr ResultEmpty result_value_empty { std::in_place, std::size_t {42} };
    constexpr ResultBase result_value_base { result_value_empty };
    constexpr ResultPrefixed result_value_prefixed { result_value_base };
    constexpr ResultUnPrefixed result_value_unprefixed { result_value_base };

    STATIC_REQUIRE(result_value_base.has_value());
    STATIC_REQUIRE(result_value_prefixed.has_value());
    STATIC_REQUIRE(result_value_unprefixed.has_value());

    STATIC_REQUIRE(result_value_base.value().value_ == 42);
    STATIC_REQUIRE(result_value_prefixed.value().value_ == 42);
    STATIC_REQUIRE(result_value_unprefixed.value().value_ == 42);

    // The active error alternative is not reindexed when the error rows share a
    // prefix but the BasicResult interface cannot validate this.

    iterate_index_array<1, 3>([]<std::size_t I>(const index_constant<I>) -> void {

        constexpr ResultBase result_base { std::in_place_type<E<I>>, std::size_t {I + 42} };
        constexpr ResultPrefixed result_prefixed { result_base };

        STATIC_REQUIRE(result_prefixed.has_error());
        STATIC_REQUIRE(result_prefixed.holds_error<E<I>>());
        STATIC_REQUIRE(result_prefixed.error<E<I>>().value() == I + 42);

    });

    // The active error alternative is reindexed when the error rows do not
    // share a prefix but the BasicResult interface cannot validate this.

    iterate_index_array<1, 3>([]<std::size_t I>(const index_constant<I>) -> void {

        constexpr ResultBase result_base { std::in_place_type<E<I>>, std::size_t {I + 42} };
        constexpr ResultUnPrefixed result_unprefixed { result_base };

        STATIC_REQUIRE(result_unprefixed.has_error());
        STATIC_REQUIRE(result_unprefixed.holds_error<E<I>>());
        STATIC_REQUIRE(result_unprefixed.error<E<I>>().value() == I + 42 );

    });

}

// Constraint specification tests.

TEMPLATE_TEST_CASE("varerr_result_constraints_default", "[varerr][result]",
    TrivialType,
    NoDefaultConstructType,
    NoCopyConstructType,
    NoMoveConstructType,
    NoCopyAssignType,
    NoMoveAssignType
) {

    using ValueType = TestType;
    using RowType = varerr::Row<NoDefaultConstructType, ThrowAllErrorType>;
    using UniverseType = pack_apply_t<bind_adapter<UniverseT>, RowType>;
    using ResultType = varerr::result_from_row_t<UniverseType, ValueType, RowType>;

    // The default constructor inherits default constructibility from the value
    // type.

    constexpr bool is_constructible = std::is_default_constructible_v<ValueType>;
    STATIC_REQUIRE(std::is_default_constructible_v<ResultType> == is_constructible);

}

TEMPLATE_TEST_CASE("varerr_result_constraints_emplace_value", "[varerr][result]",
    TrivialType,
    NoDefaultConstructType,
    NoCopyConstructType,
    NoMoveConstructType,
    NoCopyAssignType,
    NoMoveAssignType
) {

    using ValueType = TestType;
    using RowType = varerr::Row<NoDefaultConstructType, ThrowAllErrorType>;
    using UniverseType = pack_apply_t<bind_adapter<UniverseT>, RowType>;
    using ResultType = varerr::result_from_row_t<UniverseType, ValueType, RowType>;

    constexpr bool is_constructible = std::is_constructible_v<ValueType, int>;
    constexpr bool is_copy_constructible = std::is_copy_constructible_v<ValueType>;
    constexpr bool is_move_constructible = std::is_move_constructible_v<ValueType>;
    constexpr bool is_default_constructible = std::is_default_constructible_v<ValueType>;

    // The in-place value constructor is explicit.

    STATIC_REQUIRE_FALSE(std::is_convertible_v<std::in_place_t, ResultType>);
    STATIC_REQUIRE(std::is_constructible_v<ResultType, std::in_place_t> == is_default_constructible);

    // The in-place value constructor inherits constructibility from the value
    // type.

    STATIC_REQUIRE(std::is_constructible_v<ResultType, std::in_place_t, int> == is_constructible);
    STATIC_REQUIRE(std::is_constructible_v<ResultType, std::in_place_t, ValueType&> == is_copy_constructible);
    STATIC_REQUIRE(std::is_constructible_v<ResultType, std::in_place_t, ValueType&&> == is_move_constructible);
    STATIC_REQUIRE(std::is_constructible_v<ResultType, std::in_place_t, const ValueType&> == is_copy_constructible);
    STATIC_REQUIRE(std::is_constructible_v<ResultType, std::in_place_t, const ValueType&&> == is_copy_constructible);

}

TEST_CASE("varerr_result_constraints_emplace_value_types", "[varerr][result]") {

    using RowType = varerr::Row<NoDefaultConstructType, ThrowAllErrorType>;
    using UniverseType = pack_apply_t<bind_adapter<UniverseT>, RowType>;
    using ResultVoid = varerr::result_from_row_t<UniverseType, void, RowType>;
    using ResultTrivial = varerr::result_from_row_t<UniverseType, TrivialType, RowType>;

    // A void value is only in-place constructible when no arguments are given.

    STATIC_REQUIRE(std::is_constructible_v<ResultVoid, std::in_place_t>);
    STATIC_REQUIRE_FALSE(std::is_constructible_v<ResultVoid, std::in_place_t, int>);

    // The value type must be in-place constructible from the arguments.

    STATIC_REQUIRE(std::is_constructible_v<ResultTrivial, std::in_place_t, int>);
    STATIC_REQUIRE(std::is_constructible_v<ResultTrivial, std::in_place_t, TrivialType>);

    iterate_type_pack<int, TrivialType>([]<typename T>(const std::type_identity<T>) -> void {
        STATIC_REQUIRE_FALSE(std::is_constructible_v<ResultTrivial, std::in_place_t, T*>);
        STATIC_REQUIRE_FALSE(std::is_constructible_v<ResultTrivial, std::in_place_t, T, T>);
        STATIC_REQUIRE_FALSE(std::is_constructible_v<ResultTrivial, std::in_place_t, T(*)()>);
        STATIC_REQUIRE_FALSE(std::is_constructible_v<ResultTrivial, std::in_place_t, T[]>);
        STATIC_REQUIRE_FALSE(std::is_constructible_v<ResultTrivial, std::in_place_t, T[1]>);
    });

}

TEMPLATE_TEST_CASE("varerr_result_constraints_emplace_error", "[varerr][result]",
    TrivialType,
    NoDefaultConstructType,
    NoCopyAssignType,
    NoMoveAssignType
) {

    using ErrorType = TestType;
    using ValueType = DegenerateValueType;
    using RowType = varerr::Row<DegenerateErrorType, ErrorType, ThrowAllErrorType>;
    using UniverseType = pack_apply_t<bind_adapter<UniverseT>, RowType>;
    using ResultType = varerr::result_from_row_t<UniverseType, ValueType, RowType>;

    constexpr bool is_constructible = std::is_constructible_v<ErrorType, int>;
    constexpr bool is_copy_constructible = std::is_copy_constructible_v<ErrorType>;
    constexpr bool is_move_constructible = std::is_move_constructible_v<ErrorType>;
    constexpr bool is_default_constructible = std::is_default_constructible_v<ErrorType>;

    // The in-place error constructor is explicit.

    STATIC_REQUIRE_FALSE(std::is_convertible_v<std::in_place_type_t<ErrorType>, ResultType>);
    STATIC_REQUIRE(std::is_constructible_v<ResultType, std::in_place_type_t<ErrorType>> == is_default_constructible);

    // The in-place error constructor inherits constructibility from the error
    // alternative.

    STATIC_REQUIRE(std::is_constructible_v<ResultType, std::in_place_type_t<ErrorType>, int> == is_constructible);
    STATIC_REQUIRE(std::is_constructible_v<ResultType, std::in_place_type_t<ErrorType>, ErrorType&> == is_copy_constructible);
    STATIC_REQUIRE(std::is_constructible_v<ResultType, std::in_place_type_t<ErrorType>, ErrorType&&> == is_move_constructible);
    STATIC_REQUIRE(std::is_constructible_v<ResultType, std::in_place_type_t<ErrorType>, const ErrorType&> == is_copy_constructible);
    STATIC_REQUIRE(std::is_constructible_v<ResultType, std::in_place_type_t<ErrorType>, const ErrorType&&> == is_copy_constructible);

}

TEST_CASE("varerr_result_constraints_emplace_error_types", "[varerr][result]") {

    // The empty error row is never in-place constructible.

    using ResultEmptyType = varerr::result_from_row_t<UniverseI, TrivialType, varerr::Row<>>;

    STATIC_REQUIRE_FALSE(std::is_constructible_v<ResultEmptyType, std::in_place_type_t<TrivialType>>);
    STATIC_REQUIRE_FALSE(std::is_constructible_v<ResultEmptyType, std::in_place_type_t<TrivialType>, int>);
    STATIC_REQUIRE_FALSE(std::is_constructible_v<ResultEmptyType, std::in_place_type_t<TrivialType>, TrivialType>);

    // The contructed error alternative must be exact in the error row.

    using RowCollideType = varerr::Row<AliasA>;
    using ResultCollideType = varerr::result_from_row_t<UniverseAlias, TrivialType, RowCollideType>;

    iterate_type_pack<AliasA, AliasB, AliasC>([]<typename A>(const std::type_identity<A>) -> void {
        iterate_cvref_matrix<A>([]<typename T>(const std::type_identity<T>) -> void {
            constexpr bool is_alternative = varerr::row_elem_normalized_v<UniverseAlias, T, RowCollideType>;
            if constexpr (is_alternative) {
                constexpr bool is_exact = std::same_as<T, varerr::detail::row_subscript_t<
                                              varerr::row_index_normalized_v<
                                              UniverseAlias, T, RowCollideType>, RowCollideType>>;
                STATIC_REQUIRE(std::is_constructible_v<ResultCollideType, std::in_place_type_t<T>, int> == is_exact);
            } else {
                STATIC_REQUIRE_FALSE(std::is_constructible_v<ResultCollideType, std::in_place_type_t<T>, int>);
            }
        });
    });

    // The error type must be in-place constructible from the arguments.

    using RowType = varerr::Row<DegenerateErrorType, TrivialType, ThrowAllErrorType>;
    using UniverseType = pack_apply_t<bind_adapter<UniverseT>, RowType>;
    using ResultType = varerr::result_from_row_t<UniverseType, DegenerateErrorType, RowType>;

    STATIC_REQUIRE(std::is_constructible_v<ResultType, std::in_place_type_t<TrivialType>, int>);
    STATIC_REQUIRE(std::is_constructible_v<ResultType, std::in_place_type_t<TrivialType>, TrivialType>);

    iterate_type_pack<int, TrivialType>([]<typename T>(const std::type_identity<T>) -> void {
        STATIC_REQUIRE_FALSE(std::is_constructible_v<ResultType, std::in_place_type_t<TrivialType>, T*>);
        STATIC_REQUIRE_FALSE(std::is_constructible_v<ResultType, std::in_place_type_t<TrivialType>, T, T>);
        STATIC_REQUIRE_FALSE(std::is_constructible_v<ResultType, std::in_place_type_t<TrivialType>, T(*)()>);
        STATIC_REQUIRE_FALSE(std::is_constructible_v<ResultType, std::in_place_type_t<TrivialType>, T[]>);
        STATIC_REQUIRE_FALSE(std::is_constructible_v<ResultType, std::in_place_type_t<TrivialType>, T[1]>);
    });

}

TEST_CASE("varerr_result_constraints_widen", "[varerr][result]") {

    // The widening constructor is implicit.

    iterate_cref_matrix<ResultVarSubType<int>>([]<typename T>(const std::type_identity<T>) -> void {
        STATIC_REQUIRE(std::is_convertible_v<T, ResultVarSuperType<int>>);
    });

    // Volatile references are prohibited.

    iterate_cvref_matrix<ResultVarSubType<int>>([]<typename T>(const std::type_identity<T>) -> void {
        constexpr bool is_volatile = std::is_volatile_v<std::remove_reference_t<T>>;
        STATIC_REQUIRE(std::is_constructible_v<ResultVarSuperType<int>, T> == !is_volatile);
    });

    // The widening constructor only accepts a non-empty proper subset of the
    // target row. The empty row is handled by the copy and move constructors.

    STATIC_REQUIRE(std::is_constructible_v<ResultVarEmptyType<int>, ResultVarEmptyType<int>>);
    STATIC_REQUIRE(std::is_constructible_v<ResultVarSubType<int>, ResultVarEmptyType<int>>);
    STATIC_REQUIRE(std::is_constructible_v<ResultVarSubType<int>, ResultVarSubType<int>>);
    STATIC_REQUIRE(std::is_constructible_v<ResultVarSuperType<int>, ResultVarSubType<int>>);
    STATIC_REQUIRE(std::is_constructible_v<ResultVarSuperType<int>, ResultVarSuperType<int>>);
    STATIC_REQUIRE(std::is_constructible_v<ResultVarSuperType<int>, ResultVarLeftType<int>>);
    STATIC_REQUIRE(std::is_constructible_v<ResultVarSuperType<int>, ResultVarRightType<int>>);

    STATIC_REQUIRE_FALSE(std::is_constructible_v<ResultVarEmptyType<int>, ResultVarSuperType<int>>);
    STATIC_REQUIRE_FALSE(std::is_constructible_v<ResultVarSubType<int>, ResultVarSuperType<int>>);
    STATIC_REQUIRE_FALSE(std::is_constructible_v<ResultVarSubType<int>, ResultVarLeftType<int>>);
    STATIC_REQUIRE_FALSE(std::is_constructible_v<ResultVarSubType<int>, ResultVarRightType<int>>);

    // The value types must be identical.

    STATIC_REQUIRE(std::is_constructible_v<ResultVarSuperType<int>, ResultVarSubType<int>>);
    STATIC_REQUIRE_FALSE(std::is_constructible_v<ResultVarSuperType<int>, ResultVarSubType<double>>);
    STATIC_REQUIRE_FALSE(std::is_constructible_v<ResultVarSuperType<int>, ResultVarSubType<const int>>);
    STATIC_REQUIRE_FALSE(std::is_constructible_v<ResultVarSuperType<int>, ResultVarSubType<volatile int>>);
    STATIC_REQUIRE_FALSE(std::is_constructible_v<ResultVarSuperType<int>, ResultVarSubType<int*>>);
    STATIC_REQUIRE_FALSE(std::is_constructible_v<ResultVarSuperType<int>, ResultVarSubType<int(*)()>>);

    // The universe types must be identical.

    using RowEmptyType = varerr::Row<>;
    using UniverseEmptyType = pack_apply_t<bind_adapter<UniverseT>, RowEmptyType>;
    using ResultEmptyType = varerr::result_from_row_t<UniverseEmptyType, int, RowEmptyType>;

    using RowSubType = varerr::Row<E<0>>;
    using UniverseSubType = pack_apply_t<bind_adapter<UniverseT>, RowSubType>;
    using ResultSubType = varerr::result_from_row_t<UniverseSubType, int, RowSubType>;

    using RowSuperType = varerr::Row<E<0>, E<1>>;
    using UniverseSuperType = pack_apply_t<bind_adapter<UniverseT>, RowSuperType>;
    using ResultSuperType = varerr::result_from_row_t<UniverseSuperType, int, RowSuperType>;

    STATIC_REQUIRE_FALSE(std::is_constructible_v<ResultSubType, ResultEmptyType>);
    STATIC_REQUIRE_FALSE(std::is_constructible_v<ResultSuperType, ResultSubType>);

    // The widening constructor only accepts an exact proper subset of the tar-
    // get row.

    using RowAliasSubType = varerr::Row<AliasA>;
    using RowAliasSuperType = varerr::Row<AliasA, AliasC>;
    using RowAliasCollideSuperType = varerr::Row<AliasB, AliasC>;

    using ResultAliasSubType = varerr::result_from_row_t<UniverseAlias, int, RowAliasSubType>;
    using ResultAliasSuperType = varerr::result_from_row_t<UniverseAlias, int, RowAliasSuperType>;
    using ResultAliasCollideSuperType = varerr::result_from_row_t<UniverseAlias, int, RowAliasCollideSuperType>;

    STATIC_REQUIRE(std::is_constructible_v<ResultAliasSuperType, ResultAliasSubType>);
    STATIC_REQUIRE_FALSE(std::is_constructible_v<ResultAliasCollideSuperType, ResultAliasSubType>);

}

TEST_CASE("varerr_result_constraints_widen_types", "[varerr][result]") {

    // A void value type is unconditionally widenable.

    iterate_cref_matrix<ResultVarSubType<void>>([]<typename T>(const std::type_identity<T>) -> void {
        STATIC_REQUIRE(std::is_constructible_v<ResultVarSuperType<void>, T>);
    });

    // The value type must be constructible from itself.

    iterate_cref_matrix<ResultVarSubType<TrivialType>>([]<typename T>(const std::type_identity<T>) -> void {
        STATIC_REQUIRE(std::is_constructible_v<ResultVarSuperType<TrivialType>, T>);
    });

    iterate_cref_matrix<ResultVarSubType<NoCopyConstructType>>([]<typename T>(const std::type_identity<T>) -> void {
        constexpr bool is_const = std::is_const_v<std::remove_reference_t<T>>;
        constexpr bool is_rvalue = std::is_rvalue_reference_v<T&&>;
        constexpr bool is_constructible = is_rvalue && !is_const;
        STATIC_REQUIRE(std::is_constructible_v<ResultVarSuperType<NoCopyConstructType>, T> == is_constructible);
    });

    iterate_cref_matrix<ResultVarSubType<NoMoveConstructType>>([]<typename T>(const std::type_identity<T>) -> void {
        constexpr bool is_const = std::is_const_v<std::remove_reference_t<T>>;
        constexpr bool is_lvalue = std::is_lvalue_reference_v<T>;
        constexpr bool is_constructible = is_const || is_lvalue;
        STATIC_REQUIRE(std::is_constructible_v<ResultVarSuperType<NoMoveConstructType>, T> == is_constructible);
    });

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

TEMPLATE_TEST_CASE("varerr_result_constraints_has_error", "[varerr][result]",
    (HomResult<void, 0>),
    (HomResult<void, 3, 1, 2>),
    (HomResult<TrivialType, 0>),
    (HomResult<TrivialType, 3, 1, 2>)
) {

    using ResultType = TestType;

    iterate_cvref_matrix<ResultType>([]<typename T>(const std::type_identity<T>) -> void {
        constexpr bool is_volatile = std::is_volatile_v<std::remove_reference_t<T>>;
        STATIC_REQUIRE(IsResultHasErrorWellFormed<T> == !is_volatile);
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

TEMPLATE_TEST_CASE("varerr_result_constraints_take", "[varerr][result]",
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
        constexpr bool is_empty_row = varerr::row_size_v<varerr::result_row_t<T>> == 0;
        STATIC_REQUIRE(IsResultTakeWellFormed<T> == (!is_void && !is_volatile && is_empty_row));
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

TEST_CASE("varerr_result_constraints_transform", "[varerr][result]") {

    using ValueType = TrivialType;
    using ResultType = HomResult<ValueType, 1>;

    // The implicit object parameter must be non-volatile.

    iterate_cvref_matrix<ResultType>([]<typename T>(const std::type_identity<T>) -> void {
        constexpr bool is_volatile = std::is_volatile_v<std::remove_reference_t<T>>;
        STATIC_REQUIRE(IsResultTransformWellFormed<T, FunctorOnSelfFromForwardToValue<ValueType>> == !is_volatile);
    });

    // The functor must not return an lvalue reference.

    iterate_cref_matrix<ResultType>([]<typename T>(const std::type_identity<T>) -> void {
        constexpr bool is_lvalue = std::is_lvalue_reference_v<T>;
        STATIC_REQUIRE(IsResultTransformWellFormed<T, FunctorOnSelfFromForwardToForward<ValueType>> == !is_lvalue);
    });

    iterate_cref_matrix<ResultType>([]<typename T>(const std::type_identity<T>) -> void {
        constexpr bool is_rvalue = std::is_rvalue_reference_v<T&&>;
        constexpr bool is_const = std::is_const_v<std::remove_reference_t<T>>;
        STATIC_REQUIRE(IsResultTransformWellFormed<T, FunctorOnConstValueFromLeastConstrainedToValue<ValueType>>);
        STATIC_REQUIRE(IsResultTransformWellFormed<T, FunctorOnConstValueFromLeastConstrainedToConstValue<ValueType>>);
        STATIC_REQUIRE_FALSE(IsResultTransformWellFormed<T, FunctorOnConstValueFromLeastConstrainedToLValueRef<ValueType>>);
        STATIC_REQUIRE_FALSE(IsResultTransformWellFormed<T, FunctorOnConstValueFromLeastConstrainedToConstLValueRef<ValueType>>);
        STATIC_REQUIRE(IsResultTransformWellFormed<T, FunctorOnConstValueFromLeastConstrainedToRValueRef<ValueType>> == (is_rvalue && !is_const));
        STATIC_REQUIRE(IsResultTransformWellFormed<T, FunctorOnConstValueFromLeastConstrainedToConstRValueRef<ValueType>> == is_rvalue);
    });

    // A functor that accepts a value, const value or const lvalue reference
    // binds every value category.

    iterate_cref_matrix<ResultType>([]<typename T>(const std::type_identity<T>) -> void {
        STATIC_REQUIRE(IsResultTransformWellFormed<T, FunctorOnSelfFromValueToValue<ValueType>>);
        STATIC_REQUIRE(IsResultTransformWellFormed<T, FunctorOnSelfFromConstValueToValue<ValueType>>);
        STATIC_REQUIRE(IsResultTransformWellFormed<T, FunctorOnSelfFromConstLValueRefToValue<ValueType>>);
    });

    // A functor that accepts a non-const lvalue reference only binds non-const
    // lvalue references.

    iterate_cref_matrix<ResultType>([]<typename T>(const std::type_identity<T>) -> void {
        constexpr bool is_lvalue = std::is_lvalue_reference_v<T>;
        constexpr bool is_const = std::is_const_v<std::remove_reference_t<T>>;
        constexpr bool well_formed = is_lvalue && !is_const;
        STATIC_REQUIRE(IsResultTransformWellFormed<T, FunctorOnSelfFromLValueRefToValue<ValueType>> == well_formed);
    });

    // A functor that accepts a non-const rvalue reference only binds non-const
    // rvalues and rvalue references.

    iterate_cref_matrix<ResultType>([]<typename T>(const std::type_identity<T>) -> void {
        constexpr bool is_rvalue = std::is_rvalue_reference_v<T&&>;
        constexpr bool is_const = std::is_const_v<std::remove_reference_t<T>>;
        constexpr bool well_formed = is_rvalue && !is_const;
        STATIC_REQUIRE(IsResultTransformWellFormed<T, FunctorOnSelfFromRValueRefToValue<ValueType>> == well_formed);
    });

    // A functor that accepts a const rvalue reference binds every value catego-
    // ry except const and non-const lvalue references.

    iterate_cref_matrix<ResultType>([]<typename T>(const std::type_identity<T>) -> void {
        constexpr bool is_lvalue = std::is_lvalue_reference_v<T>;
        STATIC_REQUIRE(IsResultTransformWellFormed<T, FunctorOnSelfFromConstRValueRefToValue<ValueType>> == !is_lvalue);
    });

    // A nullary functor is never invocable with a non-void value.

    iterate_cvref_matrix<ResultType>([]<typename T>(const std::type_identity<T>) -> void {
        STATIC_REQUIRE_FALSE(IsResultTransformWellFormed<T, FunctorOnSelfFromVoidToValue<ValueType>>);
    });

}

TEST_CASE("varerr_result_constraints_transform_void", "[varerr][result]") {

    using ValueVoidType = void;
    using ResultVoidType = HomResult<ValueVoidType, 1>;

    using ValueTrivialType = TrivialType;
    using ResultTrivialType = HomResult<ValueTrivialType, 1>;

    // A void result accepts a nullary functor.

    iterate_cvref_matrix<ResultVoidType>([]<typename T>(const std::type_identity<T>) -> void {
        constexpr bool is_volatile = std::is_volatile_v<std::remove_reference_t<T>>;
        STATIC_REQUIRE(IsResultTransformWellFormed<T, FunctorOnSelfFromVoidToVoid> == !is_volatile);
        STATIC_REQUIRE(IsResultTransformWellFormed<T, FunctorOnSelfFromVoidToValue<ValueTrivialType>> == !is_volatile);
        STATIC_REQUIRE_FALSE(IsResultTransformWellFormed<T, FunctorOnSelfFromForwardToVoid>);
        STATIC_REQUIRE_FALSE(IsResultTransformWellFormed<T, FunctorOnSelfFromForwardToValue<ValueTrivialType>>);
    });

    // A non-void result accepts a non-void functor.

    iterate_cvref_matrix<ResultTrivialType>([]<typename T>(const std::type_identity<T>) -> void {
        constexpr bool is_volatile = std::is_volatile_v<std::remove_reference_t<T>>;
        STATIC_REQUIRE(IsResultTransformWellFormed<T, FunctorOnSelfFromForwardToVoid> == !is_volatile);
        STATIC_REQUIRE_FALSE(IsResultTransformWellFormed<T, FunctorOnSelfFromVoidToVoid>);
    });

}

TEST_CASE("varerr_result_constraints_transform_functor", "[varerr][result]") {

    using ValueType = TrivialType;
    using ResultType = HomResult<ValueType, 1>;

    // The functor must be passed with a value category and constness accepted
    // by the call operator.

    iterate_cref_matrix<FunctorOnValueFromConstLValueRefToValue<ValueType>>(
        []<typename F>(const std::type_identity<F>) -> void {
        constexpr bool is_const = std::is_const_v<std::remove_reference_t<F>>;
        STATIC_REQUIRE(IsResultTransformWellFormed<ResultType&, F> == !is_const);
    });

    iterate_cref_matrix<FunctorOnConstValueFromConstLValueRefToValue<ValueType>>(
        []<typename F>(const std::type_identity<F>) -> void {
        STATIC_REQUIRE(IsResultTransformWellFormed<ResultType&, F>);
    });

    iterate_cref_matrix<FunctorOnLValueRefFromConstLValueRefToValue<ValueType>>(
        []<typename F>(const std::type_identity<F>) -> void {
        constexpr bool is_lvalue = std::is_lvalue_reference_v<F>;
        constexpr bool is_const = std::is_const_v<std::remove_reference_t<F>>;
        STATIC_REQUIRE(IsResultTransformWellFormed<ResultType&, F> == (is_lvalue && !is_const));
    });

    iterate_cref_matrix<FunctorOnConstLValueRefFromConstLValueRefToValue<ValueType>>(
        []<typename F>(const std::type_identity<F>) -> void {
        STATIC_REQUIRE(IsResultTransformWellFormed<ResultType&, F>);
    });

    iterate_cref_matrix<FunctorOnRValueRefFromConstLValueRefToValue<ValueType>>(
        []<typename F>(const std::type_identity<F>) -> void {
        constexpr bool is_rvalue = std::is_rvalue_reference_v<F&&>;
        constexpr bool is_const = std::is_const_v<std::remove_reference_t<F>>;
        STATIC_REQUIRE(IsResultTransformWellFormed<ResultType&, F> == (is_rvalue && !is_const));
    });

    iterate_cref_matrix<FunctorOnConstRValueRefFromConstLValueRefToValue<ValueType>>(
        []<typename F>(const std::type_identity<F>) -> void {
        constexpr bool is_rvalue = std::is_rvalue_reference_v<F&&>;
        STATIC_REQUIRE(IsResultTransformWellFormed<ResultType&, F> == is_rvalue);
    });

}

TEST_CASE("varerr_result_constraints_transform_construct", "[varerr][result]") {

    using ValueType = TrivialType;
    using ResultType = HomResult<ValueType, 1>;

    // This test exercises the constructibility constraint, which asserts that
    // the unqualified result type R must be constructible from the (possibly)
    // qualified result type.

    iterate_cref_matrix<ResultType>([]<typename T>(const std::type_identity<T>) -> void {
        iterate_cref_matrix<NoMoveConstructType>([]<typename R>(const std::type_identity<R>) -> void {
            constexpr bool is_rvalue = std::is_rvalue_reference_v<R&&>;
            constexpr bool is_const = std::is_const_v<std::remove_reference_t<R>>;
            constexpr bool well_formed = is_rvalue && is_const;
            STATIC_REQUIRE(IsResultTransformWellFormed<T, FunctorOnSelfFromForwardToDeclared<R>> == well_formed);
        });
    });

    iterate_cref_matrix<ResultType>([]<typename T>(const std::type_identity<T>) -> void {
        iterate_cref_matrix<NoCopyConstructType>([]<typename R>(const std::type_identity<R>) -> void {
            constexpr bool is_rvalue = std::is_rvalue_reference_v<R&&>;
            constexpr bool is_const = std::is_const_v<std::remove_reference_t<R>>;
            constexpr bool well_formed = is_rvalue && !is_const;
            STATIC_REQUIRE(IsResultTransformWellFormed<T, FunctorOnSelfFromForwardToDeclared<R>> == well_formed);
        });
    });

}

// Return value specification tests.

TEST_CASE("varerr_result_return_has_value", "[varerr][result]") {
    REQUIRE(false); /* trivial */
}

TEST_CASE("varerr_result_return_has_error", "[varerr][result]") {
    REQUIRE(false); /* trivial */
}

TEST_CASE("varerr_result_return_holds_error", "[varerr][result]") {
    REQUIRE(false); /* trivial */
}

TEST_CASE("varerr_result_return_value_if", "[varerr][result]") {

    using ValueType = TrivialType;
    using ResultType = HomResult<ValueType, 1>;

    iterate_cref_matrix<ResultType>([]<typename T>(const std::type_identity<T>) -> void {
        constexpr bool is_lvalue_ref = std::is_lvalue_reference_v<T>;
        constexpr bool is_const = std::is_const_v<std::remove_reference_t<T>>;
        if constexpr (is_lvalue_ref) {
            using ExpectedType = std::conditional_t<is_const, const ValueType*, ValueType*>;
            using ReturnType = decltype(std::declval<T>().value_if());
            STATIC_REQUIRE(std::same_as<ReturnType, ExpectedType>);
        }
    });

}

TEST_CASE("varerr_result_return_value", "[varerr][result]") {

    using ValueType = TrivialType;
    using ResultType = HomResult<ValueType, 1>;

    iterate_cref_matrix<ResultType>([]<typename T>(const std::type_identity<T>) -> void {
        constexpr bool is_lvalue_ref = std::is_lvalue_reference_v<T>;
        constexpr bool is_const = std::is_const_v<std::remove_reference_t<T>>;
        using ExpectedBaseType = std::conditional_t<is_const, const ValueType, ValueType>;
        using ExpectedType = std::conditional_t<is_lvalue_ref, ExpectedBaseType&, ExpectedBaseType&&>;
        using ReturnType = decltype(std::declval<T>().value());
        STATIC_REQUIRE(std::same_as<ReturnType, ExpectedType>);
    });

}

TEST_CASE("varerr_result_return_take", "[varerr][result]") {

    using ValueType = TrivialType;
    using ResultType = HomResult<ValueType, 0>;

    iterate_cref_matrix<ResultType>([]<typename T>(const std::type_identity<T>) -> void {
        constexpr bool is_lvalue_ref = std::is_lvalue_reference_v<T>;
        constexpr bool is_const = std::is_const_v<std::remove_reference_t<T>>;
        using ExpectedBaseType = std::conditional_t<is_const, const ValueType, ValueType>;
        using ExpectedType = std::conditional_t<is_lvalue_ref, ExpectedBaseType&, ExpectedBaseType&&>;
        using ReturnType = decltype(std::declval<T>().take());
        STATIC_REQUIRE(std::same_as<ReturnType, ExpectedType>);
    });

}

TEST_CASE("varerr_result_return_error_if", "[varerr][result]") {

    using ErrorType = E<0>;
    using ValueType = TrivialType;
    using ResultType = HomResult<ValueType, 1>;

    iterate_cref_matrix<ResultType>([]<typename T>(const std::type_identity<T>) -> void {
        constexpr bool is_lvalue_ref = std::is_lvalue_reference_v<T>;
        constexpr bool is_const = std::is_const_v<std::remove_reference_t<T>>;
        if constexpr (is_lvalue_ref) {
            using ExpectedType = std::conditional_t<is_const, const ErrorType*, ErrorType*>;
            using ReturnType = decltype(std::declval<T>().template error_if<ErrorType>());
            STATIC_REQUIRE(std::same_as<ReturnType, ExpectedType>);
        }
    });

}

TEST_CASE("varerr_result_return_error", "[varerr][result]") {

    using ErrorType = E<0>;
    using ValueType = TrivialType;
    using ResultType = HomResult<ValueType, 1>;

    iterate_cref_matrix<ResultType>([]<typename T>(const std::type_identity<T>) -> void {
        constexpr bool is_lvalue_ref = std::is_lvalue_reference_v<T>;
        constexpr bool is_const = std::is_const_v<std::remove_reference_t<T>>;
        if constexpr (is_lvalue_ref) {
            using ExpectedType = std::conditional_t<is_const, const ErrorType&, ErrorType&>;
            using ReturnType = decltype(std::declval<T>().template error<ErrorType>());
            STATIC_REQUIRE(std::same_as<ReturnType, ExpectedType>);
        }
    });

}

TEST_CASE("varerr_result_return_transform", "[varerr][result]") {
    REQUIRE(false);
}

// Exception specification tests.

TEST_CASE("varerr_result_noexcept_default", "[varerr][result]") {

    using ResultDefType = HomResult<TrivialType, 1>;
    using ResultDefVoidType = HomResult<void, 1>;
    using ResultDefThrowType = HomResult<ThrowDefaultConstructType, 1>;

    // The default constructor inherits nothrow default constructibility from
    // the value type.

    STATIC_REQUIRE(std::is_nothrow_default_constructible_v<ResultDefType>);
    STATIC_REQUIRE(std::is_nothrow_default_constructible_v<ResultDefVoidType>);
    STATIC_REQUIRE_FALSE(std::is_nothrow_default_constructible_v<ResultDefThrowType>);

}

TEST_CASE("varerr_result_noexcept_emplace_value", "[varerr][result]") {

    using ResultVoidType = InvResult<void, ThrowAllErrorType>;
    using ResultThrowType = InvResult<ConditionalThrowType, ThrowAllErrorType>;

    // The in-place value constructor inherits nothrow constructibility from the
    // value (but not the error) type.

    STATIC_REQUIRE(std::is_constructible_v<ResultThrowType, std::in_place_t, int>);
    STATIC_REQUIRE(std::is_constructible_v<ResultThrowType, std::in_place_t, double>);

    STATIC_REQUIRE(std::is_nothrow_constructible_v<ConditionalThrowType, int>);
    STATIC_REQUIRE_FALSE(std::is_nothrow_constructible_v<ConditionalThrowType, double>);

    STATIC_REQUIRE(std::is_nothrow_constructible_v<ResultThrowType, std::in_place_t, int>);
    STATIC_REQUIRE_FALSE(std::is_nothrow_constructible_v<ResultThrowType, std::in_place_t, double>);

    // The in-place value constructor is unconditionally noexcept.

    STATIC_REQUIRE(std::is_nothrow_constructible_v<ResultVoidType, std::in_place_t>);

}

TEST_CASE("varerr_result_noexcept_emplace_error", "[varerr][result]") {

    // std::is_nothrow_constructible requires nothrow destructibility on every
    // major implementation (LWG 2116). This prevents ThrowAllValueType from
    // being usable for the non-throwing branch of the test.

    using ResultThrowType = InvResult<ThrowAllButDestructValueType, ThrowAllErrorType, ConditionalThrowType>;

    // The in-place error constructor inherits nothrow constructibility from the
    // error (but not the value) type.

    STATIC_REQUIRE(std::is_constructible_v<ResultThrowType, std::in_place_type_t<ConditionalThrowType>, int>);
    STATIC_REQUIRE(std::is_constructible_v<ResultThrowType, std::in_place_type_t<ConditionalThrowType>, double>);

    STATIC_REQUIRE(std::is_nothrow_constructible_v<ConditionalThrowType, int>);
    STATIC_REQUIRE_FALSE(std::is_nothrow_constructible_v<ConditionalThrowType, double>);

    STATIC_REQUIRE(std::is_nothrow_constructible_v<ResultThrowType, std::in_place_type_t<ConditionalThrowType>, int>);
    STATIC_REQUIRE_FALSE(std::is_nothrow_constructible_v<ResultThrowType, std::in_place_type_t<ConditionalThrowType>, double>);

}

TEMPLATE_TEST_CASE("varerr_result_noexcept_widen", "[varerr][result]",
    (varerr::Row<>),
    (varerr::Row<ThrowAllErrorType>)
) {

    using RowThrowBase = TestType;
    using RowThrowExtend = varerr::Row<E<0>, ThrowAllErrorType, E<1>>;
    using UniverseThrow = pack_apply_t<bind_adapter<UniverseT>, RowThrowExtend>;

    // The widening constructor is unconditionally noexcept when the value type
    // is void.

    using ResultVoidBase = varerr::result_from_row_t<UniverseThrow, void, RowThrowBase>;
    using ResultVoidExtend = varerr::result_from_row_t<UniverseThrow, void, RowThrowExtend>;

    iterate_cref_matrix<ResultVoidBase>([]<typename T>(const std::type_identity<T>) -> void {
        STATIC_REQUIRE(noexcept(ResultVoidExtend(std::declval<T>())));
    });

    // The widening constructor inherits nothrow constructibility from the for-
    // warded value type when the value type is non-void.

    using ResultTrivialBase = varerr::result_from_row_t<UniverseThrow, TrivialType, RowThrowBase>;
    using ResultTrivialExtend = varerr::result_from_row_t<UniverseThrow, TrivialType, RowThrowExtend>;

    iterate_cref_matrix<ResultTrivialBase>([]<typename T>(const std::type_identity<T>) -> void {
        STATIC_REQUIRE(noexcept(ResultTrivialExtend(std::declval<T>())));
    });

    using ResultThrowCopyConstructBase = varerr::result_from_row_t<UniverseThrow, ThrowCopyConstructType, RowThrowBase>;
    using ResultThrowCopyConstructExtend = varerr::result_from_row_t<UniverseThrow, ThrowCopyConstructType, RowThrowExtend>;

    iterate_cref_matrix<ResultThrowCopyConstructBase>([]<typename T>(const std::type_identity<T>) -> void {
        constexpr bool is_const = std::is_const_v<std::remove_reference_t<T>>;
        constexpr bool is_lvalue_ref = std::is_lvalue_reference_v<T>;
        constexpr bool is_copied = is_const || is_lvalue_ref;
        STATIC_REQUIRE(noexcept(ResultThrowCopyConstructExtend(std::declval<T>())) == !is_copied);
    });

    using ResultThrowMoveConstructBase = varerr::result_from_row_t<UniverseThrow, ThrowMoveConstructType, RowThrowBase>;
    using ResultThrowMoveConstructExtend = varerr::result_from_row_t<UniverseThrow, ThrowMoveConstructType, RowThrowExtend>;

    iterate_cref_matrix<ResultThrowMoveConstructBase>([]<typename T>(const std::type_identity<T>) -> void {
        constexpr bool is_const = std::is_const_v<std::remove_reference_t<T>>;
        constexpr bool is_lvalue_ref = std::is_lvalue_reference_v<T>;
        constexpr bool is_copied = is_const || is_lvalue_ref;
        STATIC_REQUIRE(noexcept(ResultThrowMoveConstructExtend(std::declval<T>())) == is_copied);
    });

}

TEMPLATE_TEST_CASE("varerr_result_noexcept_has_value", "[varerr][result]",
    (InvResult<ThrowAllValueType>),
    (InvResult<ThrowAllValueType, ThrowAllErrorType>)
) {

    using ResultType = TestType;

    iterate_cref_matrix<ResultType>([]<typename T>(const std::type_identity<T>) -> void {
        STATIC_REQUIRE(noexcept(std::declval<T>().has_value()));
    });

}

TEMPLATE_TEST_CASE("varerr_result_noexcept_has_error", "[varerr][result]",
    (InvResult<ThrowAllValueType>),
    (InvResult<ThrowAllValueType, ThrowAllErrorType>)
) {

    using ResultType = TestType;

    iterate_cref_matrix<ResultType>([]<typename T>(const std::type_identity<T>) -> void {
        STATIC_REQUIRE(noexcept(std::declval<T>().has_error()));
    });

}

TEMPLATE_TEST_CASE("varerr_result_noexcept_holds_error", "[varerr][result]",
    (InvResult<ThrowAllValueType, ThrowAllErrorType>)
) {

    using ResultType = TestType;
    using ErrorType = varerr::result_alternative_t<ResultType, 0>;

    iterate_cref_matrix<ResultType>([]<typename T>(const std::type_identity<T>) -> void {
        STATIC_REQUIRE(noexcept(std::declval<T>().template holds_error<ErrorType>()));
    });

}

TEMPLATE_TEST_CASE("varerr_result_noexcept_value_if", "[varerr][result]",
    (InvResult<ThrowAllValueType>),
    (InvResult<ThrowAllValueType, ThrowAllErrorType>)
) {

    using ResultType = TestType;

    iterate_cref_matrix<ResultType>([]<typename T>(const std::type_identity<T>) -> void {
        constexpr bool is_lvalue_ref = std::is_lvalue_reference_v<T>;
        if constexpr (is_lvalue_ref) {
            STATIC_REQUIRE(noexcept(std::declval<T>().value_if()));
        }
    });

}

TEMPLATE_TEST_CASE("varerr_result_noexcept_value", "[varerr][result]",
    (InvResult<ThrowAllValueType>),
    (InvResult<ThrowAllValueType, ThrowAllErrorType>)
) {

    using ResultType = TestType;

    iterate_cref_matrix<ResultType>([]<typename T>(const std::type_identity<T>) -> void {
        STATIC_REQUIRE(noexcept(std::declval<T>().value()));
    });

}

TEMPLATE_TEST_CASE("varerr_result_noexcept_take", "[varerr][result]",
    (InvResult<ThrowAllValueType>)
) {

    using ResultType = TestType;

    iterate_cref_matrix<ResultType>([]<typename T>(const std::type_identity<T>) -> void {
        STATIC_REQUIRE(noexcept(std::declval<T>().take()));
    });

}

TEMPLATE_TEST_CASE("varerr_result_noexcept_error_if", "[varerr][result]",
    (InvResult<ThrowAllValueType, ThrowAllErrorType>)
) {

    using ResultType = TestType;
    using ErrorType = varerr::result_alternative_t<ResultType, 0>;

    iterate_cref_matrix<ResultType>([]<typename T>(const std::type_identity<T>) -> void {
        constexpr bool is_lvalue_ref = std::is_lvalue_reference_v<T>;
        if constexpr (is_lvalue_ref) {
            STATIC_REQUIRE(noexcept(std::declval<T>().template error_if<ErrorType>()));
        }
    });

}

TEMPLATE_TEST_CASE("varerr_result_noexcept_error", "[varerr][result]",
    (InvResult<ThrowAllValueType, ThrowAllErrorType>)
) {

    using ResultType = TestType;
    using ErrorType = varerr::result_alternative_t<ResultType, 0>;

    iterate_cref_matrix<ResultType>([]<typename T>(const std::type_identity<T>) -> void {
        constexpr bool is_lvalue_ref = std::is_lvalue_reference_v<T>;
        if constexpr (is_lvalue_ref) {
            STATIC_REQUIRE(noexcept(std::declval<T>().template error<ErrorType>()));
        }
    });

}

TEST_CASE("varerr_result_noexcept_transform", "[varerr][result]") {
    REQUIRE(false);
}

// Functional tests

TEST_CASE("varerr_result_functional_default", "[varerr][result]") {
    REQUIRE(false); /* default constructor */
}

TEST_CASE("varerr_result_functional_emplace_value", "[varerr][result]") {
    REQUIRE(false); /* in-place value constructor */
}

TEST_CASE("varerr_result_functional_emplace_error", "[varerr][result]") {
    REQUIRE(false); /* in-place error constructor */
}

TEST_CASE("varerr_result_functional_widen", "[varerr][result]") {
    REQUIRE(false); /* in-place error constructor */
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

TEST_CASE("varerr_result_functional_value_if", "[varerr][result]") {
    REQUIRE(false);
}

TEST_CASE("varerr_result_functional_value", "[varerr][result]") {
    REQUIRE(false);
}

TEST_CASE("varerr_result_functional_take") {
    REQUIRE(false);
}

TEST_CASE("varerr_result_functional_error_if") {
    REQUIRE(false);
}

TEST_CASE("varerr_result_functional_error") {
    REQUIRE(false);
}

TEST_CASE("varerr_result_functional_transform", "[varerr][result]") {
    REQUIRE(false);
}
