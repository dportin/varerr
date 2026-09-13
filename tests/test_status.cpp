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

// Determine whether a call is well-formed.

template <typename S, typename E>
concept IsStatusHoldsWellFormed = requires {
    std::declval<S>().template holds<E>();
};

template <typename S>
concept IsStatusIndexWellFormed = requires {
    std::declval<S>().index();
};

template <typename S, typename E>
concept IsStatusGetWellFormed = requires {
    std::declval<S>().template get<E>();
};

template <typename S, typename E>
concept IsStatusGetIfWellFormed = requires {
    std::declval<S>().template get_if<E>();
};

template <typename S, typename V>
concept IsStatusVisitWellFormed = requires {
    std::declval<S>().visit(std::declval<V>());
};

// VisitorVoidConstL binds every value type.

struct VisitorVoidL {
    [[maybe_unused]] void operator()(auto&) const noexcept {}
};

struct VisitorVoidConstL {
    [[maybe_unused]] void operator()(const auto&) const noexcept {}
};

struct VisitorVoidR {
    [[maybe_unused]] void operator()(auto&&) const noexcept {}
};

struct VisitorVoidConstR {
    [[maybe_unused]] void operator()(const auto&&) const noexcept {}
};

struct VisitorNonUniformL {
    [[maybe_unused]] auto operator()(auto& e) const noexcept { return e; }
};

struct VisitorNonUniformConstL {
    [[maybe_unused]] auto operator()(const auto& e) const noexcept { return e; }
};

struct VisitorNonUniformR {
    [[maybe_unused]] auto operator()(auto&& e) const noexcept { return e; }
};

struct VisitorNonUniformConstR {
    [[maybe_unused]] auto operator()(const auto&& e) const noexcept { return e; }
};

struct VisitorUniformL {
    [[maybe_unused]] std::size_t operator()(auto& e) const noexcept { return e.value(); }
};

struct VisitorUniformConstL {
    [[maybe_unused]] std::size_t operator()(const auto& e) const noexcept { return e.value(); }
};

struct VisitorUniformR {
    [[maybe_unused]] std::size_t operator()(auto&& e) const noexcept { return e.value(); }
};

struct VisitorUniformConstR {
    [[maybe_unused]] std::size_t operator()(const auto&& e) const noexcept { return e.value(); }
};

template <std::size_t I>
struct VisitorVoidPartialL {
    [[maybe_unused]] void operator()(E<I>&) const noexcept {}
};

template <std::size_t I>
struct VisitorVoidPartialConstL {
    [[maybe_unused]] void operator()(const E<I>&) const noexcept {}
};

template <std::size_t I>
struct VisitorVoidPartialR {
    [[maybe_unused]] void operator()(E<I>&&) const noexcept {}
};

template <std::size_t I>
struct VisitorVoidPartialConstR {
    [[maybe_unused]] void operator()(const E<I>&&) const noexcept {}
};

// Check whether references propagate through visitors.

struct VisitorRefPassThruL {
    [[maybe_unused]] decltype(auto) operator()(auto& e) const noexcept { return (e.value_); }
};

struct VisitorRefPassThruConstL {
    [[maybe_unused]] decltype(auto) operator()(const auto& e) const noexcept { return (e.value_); }
};

// Check whether noexcept propagates across alternatives.

template <std::size_t I>
struct VisitorThrowOnIndexConstL {
    [[maybe_unused]] void operator()(const E<I>&) const {}
    [[maybe_unused]] void operator()(const auto&) const noexcept {}
};

// Differentiates const from non-const lvalue reference.

struct VisitorThrowOnConstL {
    [[maybe_unused]] void operator()(auto&) const noexcept {}
    [[maybe_unused]] void operator()(auto&&) const noexcept {}
    [[maybe_unused]] void operator()(const auto&) const {}
    [[maybe_unused]] void operator()(const auto&&) const noexcept {}
};

// Differentiates lvalue from rvalue references.

struct VisitorThrowOnR {
    [[maybe_unused]] void operator()(auto&) const noexcept {}
    [[maybe_unused]] void operator()(auto&&) const {}
    [[maybe_unused]] void operator()(const auto&) const noexcept {}
    [[maybe_unused]] void operator()(const auto&&) const noexcept {}
};

// Differentiate constness and value category of visitor.

struct VisitorAsLValueVoid {
    [[maybe_unused]] void operator()(const auto&) & noexcept {}
};

struct VisitorAsConstLValueVoid {
    [[maybe_unused]] void operator()(const auto&) const & noexcept {}
};

struct VisitorAsRValueVoid {
    [[maybe_unused]] void operator()(const auto&) && noexcept {}
};

struct VisitorAsConstRValueVoid {
    [[maybe_unused]] void operator()(const auto&) const && noexcept {}
};

} // namespace

TEMPLATE_TEST_CASE("varerr_status_trivial", "[varerr][status]",
    HomStatus<0>, HomStatus<3>, (HetStatus<0, 3>), (HetStatus<3, 3>)
) {

    // BasicStatus inherits triviality from Storage.

    STATIC_REQUIRE(std::is_trivially_copyable_v<TestType>);
    STATIC_REQUIRE(std::is_trivially_destructible_v<TestType>);
    STATIC_REQUIRE(std::is_trivially_copy_constructible_v<TestType>);
    STATIC_REQUIRE(std::is_trivially_move_constructible_v<TestType>);
    STATIC_REQUIRE(std::is_trivially_copy_assignable_v<TestType>);
    STATIC_REQUIRE(std::is_trivially_move_assignable_v<TestType>);

}

TEST_CASE("varerr_status_trivial_default", "[varerr][status]") {

    using TrivialStatus = varerr::Status<UniverseI, TrivialType>;

    // BasicStatus is never trivially default constructible.

    STATIC_REQUIRE_FALSE(std::is_trivially_default_constructible_v<HomStatus<1>>);
    STATIC_REQUIRE_FALSE(std::is_trivially_default_constructible_v<TrivialStatus>);

}

TEST_CASE("varerr_status_construct_empty", "[varerr][status]") {

    // The empty BasicStatus is never default constructible.

    STATIC_REQUIRE_FALSE(std::is_default_constructible_v<HomStatus<0>>);

    // The empty BasicStatus has defined copy and move constructors and assign-
    // ment operators to satisfy the std::expected constraints in BasicResult.

    STATIC_REQUIRE(std::is_nothrow_copy_constructible_v<HomStatus<0>>);
    STATIC_REQUIRE(std::is_nothrow_move_constructible_v<HomStatus<0>>);
    STATIC_REQUIRE(std::is_nothrow_copy_assignable_v<HomStatus<0>>);
    STATIC_REQUIRE(std::is_nothrow_move_assignable_v<HomStatus<0>>);

}

TEST_CASE("varerr_status_construct_default", "[varerr][status]") {

    using R0 = varerr::Row<E<3>, E<1>>;
    using B0 = varerr::status_from_row_t<UniverseE, R0>;

    // The default constructor default-constructs the first alternative.

    constexpr B0 status {};

    STATIC_REQUIRE(status.holds<E<1>>());
    STATIC_REQUIRE(status.get<E<1>>().value() == 0);
    STATIC_REQUIRE(std::same_as<varerr::status_alternative_t<B0, status.index()>, E<1>>);

}

TEST_CASE("varerr_status_construct_emplace", "[varerr][status]") {

    using R0 = varerr::Row<E<3>, E<1>, E<5>>;
    using B0 = varerr::status_from_row_t<UniverseE, R0>;

    // The in-place constructor selects the alternative by type.

    iterate_index_array<1, 3, 5>([]<std::size_t I>(const index_constant<I>) -> void {

        constexpr B0 status { std::in_place_type<E<I>>, std::size_t {I + 42} };

        STATIC_REQUIRE(status.holds<E<I>>());
        STATIC_REQUIRE(status.get<E<I>>().value() == I + 42);
        STATIC_REQUIRE(std::same_as<varerr::status_alternative_t<B0, status.index()>, E<I>>);

    });

    // The in-place constructor value-initializes the alternative when no argum-
    // ents are supplied.

    iterate_index_array<1, 3, 5>([]<std::size_t I>(const index_constant<I>) -> void {

        constexpr B0 status { std::in_place_type<E<I>> };

        STATIC_REQUIRE(status.holds<E<I>>());
        STATIC_REQUIRE(status.get<E<I>>().value() == 0);
        STATIC_REQUIRE(std::same_as<varerr::status_alternative_t<B0, status.index()>, E<I>>);

    });

}

TEST_CASE("varerr_status_construct_emplace_forward", "[varerr][status]") {

    using R0 = varerr::Row<E<0>, ForwardProbeType, E<1>>;
    using U0 = pack_apply_t<bind_adapter<UniverseT>, R0>;
    using B0 = varerr::status_from_row_t<U0, R0>;

    // The in-place constructor forwards its arguments (non-uniformly).

    STATIC_REQUIRE([]() -> ForwardCategory {
        int fst = 0;
        B0 status { std::in_place_type<ForwardProbeType>, fst };
        return status.get<ForwardProbeType>().fst_;
    }() == ForwardCategory::LValue);

    STATIC_REQUIRE([]() -> ForwardCategory {
        const int fst = 0;
        B0 status { std::in_place_type<ForwardProbeType>, fst };
        return status.get<ForwardProbeType>().fst_;
    }() == ForwardCategory::ConstLValue);

    STATIC_REQUIRE([]() -> ForwardCategory {
        int fst = 0;
        B0 status { std::in_place_type<ForwardProbeType>, std::move(fst) }; // NOLINT
        return status.get<ForwardProbeType>().fst_;
    }() == ForwardCategory::RValue);

    STATIC_REQUIRE([]() -> ForwardCategory {
        const int fst = 0;
        B0 status { std::in_place_type<ForwardProbeType>, std::move(fst) }; // NOLINT
        return status.get<ForwardProbeType>().fst_;
    }() == ForwardCategory::ConstRValue);

    STATIC_REQUIRE([]() -> std::pair<ForwardCategory, ForwardCategory> {
        int fst = 0; const int snd = 1;
        B0 status { std::in_place_type<ForwardProbeType>, fst, std::move(snd) }; // NOLINT
        return { status.get<ForwardProbeType>().fst_, status.get<ForwardProbeType>().snd_ };
    }() == std::pair { ForwardCategory::LValue, ForwardCategory::ConstRValue });

    STATIC_REQUIRE([]() -> std::pair<ForwardCategory, ForwardCategory> {
        const int fst = 0; int snd = 0;
        B0 status { std::in_place_type<ForwardProbeType>, fst, std::move(snd) }; // NOLINT
        return { status.get<ForwardProbeType>().fst_, status.get<ForwardProbeType>().snd_ };
    }() == std::pair { ForwardCategory::ConstLValue, ForwardCategory::RValue });

}

TEST_CASE("varerr_status_construct_widen", "[varerr][status]") {

    using RowBase = varerr::Row<E<1>, E<3>>;
    using RowPrefixed = varerr::Row<E<1>, E<3>, E<5>>;
    using RowUnPrefixed = varerr::Row<E<0>, E<1>, E<2>, E<3>, E<4>>;

    using StatusBase = varerr::status_from_row_t<UniverseE, RowBase>;
    using StatusPrefixed = varerr::status_from_row_t<UniverseE, RowPrefixed>;
    using StatusUnPrefixed = varerr::status_from_row_t<UniverseE, RowUnPrefixed>;

    // The active member is not reindexed when the rows share a prefix.

    iterate_index_array<1, 3>([]<std::size_t I>(const index_constant<I>) -> void {

        constexpr StatusBase status_base { std::in_place_type<E<I>>, std::size_t {42} };
        constexpr StatusPrefixed status_prefixed { status_base };

        STATIC_REQUIRE(status_prefixed.holds<E<I>>());
        STATIC_REQUIRE(status_prefixed.get<E<I>>().value() == 42);
        STATIC_REQUIRE(std::same_as<varerr::status_alternative_t<StatusPrefixed, status_prefixed.index()>, E<I>>);

    });

    // The active member must be reindexed when the rows do not share a prefix.

    iterate_index_array<1, 3>([]<std::size_t I>(const index_constant<I>) -> void {

        constexpr StatusBase status_base { std::in_place_type<E<I>>, std::size_t {42} };
        constexpr StatusUnPrefixed status_unprefixed { status_base };

        STATIC_REQUIRE(status_unprefixed.holds<E<I>>());
        STATIC_REQUIRE(status_unprefixed.get<E<I>>().value() == 42);
        STATIC_REQUIRE(std::same_as<varerr::status_alternative_t<StatusUnPrefixed, status_unprefixed.index()>, E<I>>);

    });

}

TEST_CASE("varerr_status_assign", "[varerr][status]") {

    using R0 = varerr::Row<E<1>, E<3>>;
    using B0 = varerr::status_from_row_t<UniverseE, R0>;

    // Copy assignment preserves the discriminator and active alternative.

    B0 source_copy { std::in_place_type<E<3>>, std::size_t {42} };
    B0 target_copy { std::in_place_type<E<1>>, std::size_t {43} };
    target_copy = source_copy; // copy assignment

    REQUIRE(target_copy.holds<E<3>>());
    REQUIRE(target_copy.get<E<3>>().value() == 42);
    REQUIRE(target_copy.index() == source_copy.index());

    // Move assignment preserves the discriminator and active alternative.

    B0 source_move { std::in_place_type<E<3>>, std::size_t {42} };
    B0 target_move { std::in_place_type<E<1>>, std::size_t {43} };
    target_move = std::move(source_move); // NOLINT

    REQUIRE(target_move.holds<E<3>>());
    REQUIRE(target_move.get<E<3>>().value() == 42);
    REQUIRE(target_move.index() == source_move.index());

}

TEST_CASE("varerr_status_assign_widen", "[varerr][status]") {

    using RowBase = varerr::Row<E<1>, E<3>>;
    using RowPrefixed = varerr::Row<E<1>, E<3>, E<5>>;
    using RowUnPrefixed = varerr::Row<E<0>, E<1>, E<2>, E<3>, E<4>>;

    using StatusBase = varerr::status_from_row_t<UniverseE, RowBase>;
    using StatusPrefixed = varerr::status_from_row_t<UniverseE, RowPrefixed>;
    using StatusUnPrefixed = varerr::status_from_row_t<UniverseE, RowUnPrefixed>;

    // Narrowing assignment is rejected for every cvref-qualification of the
    // source. Identity assignment is accepted for non-volatile sources.

    iterate_cvref_matrix<StatusBase>([]<typename T>(const std::type_identity<T>) -> void {
        constexpr bool well_formed = !std::is_volatile_v<std::remove_reference_t<T>>;
        STATIC_REQUIRE(std::is_assignable_v<StatusBase&, T> == well_formed);
    });

    iterate_cvref_matrix<StatusPrefixed>([]<typename T>(const std::type_identity<T>) -> void {
        STATIC_REQUIRE_FALSE(std::is_assignable_v<StatusBase&, T>);
    });

    // Widening copy assignment preserves the index and active alternative when
    // the active alternative is part of shared prefix.

    StatusBase source_copy_prefix { std::in_place_type<E<3>>, std::size_t {42} };
    StatusPrefixed target_copy_prefix { std::in_place_type<E<1>>, std::size_t {43} };
    target_copy_prefix = source_copy_prefix;

    REQUIRE(target_copy_prefix.holds<E<3>>());
    REQUIRE(target_copy_prefix.get<E<3>>().value() == 42);
    REQUIRE(target_copy_prefix.index() == varerr::row_index_normalized_v<UniverseE, E<3>, RowBase>);

    // Widening move assignment preserves the index and active alternative when
    // the active alternative is part of a shared prefix.

    StatusBase source_move_prefix { std::in_place_type<E<3>>, std::size_t {42} };
    StatusPrefixed target_move_prefix { std::in_place_type<E<1>>, std::size_t {43} };
    target_move_prefix = std::move(source_move_prefix); // NOLINT

    REQUIRE(target_move_prefix.holds<E<3>>());
    REQUIRE(target_move_prefix.get<E<3>>().value() == 42);
    REQUIRE(target_move_prefix.index() == varerr::row_index_normalized_v<UniverseE, E<3>, RowBase>);

    // Widening copy assignment preserves the active alternative but not the in-
    // dex when the active alternative is not part of a shared prefix.

    StatusBase source_copy_noprefix { std::in_place_type<E<3>>, std::size_t {42} };
    StatusUnPrefixed target_copy_noprefix { std::in_place_type<E<1>>, std::size_t {43} };
    target_copy_noprefix = source_copy_noprefix;

    REQUIRE(target_copy_noprefix.holds<E<3>>());
    REQUIRE(target_copy_noprefix.get<E<3>>().value() == 42);
    REQUIRE(target_copy_noprefix.index() == varerr::row_index_normalized_v<UniverseE, E<3>, RowUnPrefixed>);

    // Widening move assignment preserves the active alternative but not the in-
    // dex when the active alternative is not part of a shared prefix.

    StatusBase source_move_noprefix { std::in_place_type<E<3>>, std::size_t {42} };
    StatusUnPrefixed target_move_noprefix { std::in_place_type<E<1>>, std::size_t {43} };
    target_move_noprefix = std::move(source_move_noprefix); // NOLINT

    REQUIRE(target_move_noprefix.holds<E<3>>());
    REQUIRE(target_move_noprefix.get<E<3>>().value() == 42);
    REQUIRE(target_move_noprefix.index() == varerr::row_index_normalized_v<UniverseE, E<3>, RowUnPrefixed>);

}

TEST_CASE("varerr_status_constraints_default", "[varerr][status]") {

    using RowDefCon = varerr::Row<E<0>, NoDefaultConstructType>;
    using RowNotDefCon = varerr::Row<NoDefaultConstructType, E<0>>;

    struct UniDefCon : pack_apply_t<bind_adapter<UniverseT>, RowDefCon> {};
    struct UniNotDefCon : pack_apply_t<bind_adapter<UniverseT>, RowNotDefCon> {};

    using StatusDefCon = varerr::status_from_row_t<UniDefCon, RowDefCon>;
    using StatusNotDefCon = varerr::status_from_row_t<UniNotDefCon, RowNotDefCon>;

    // BasicStatus inherits default constructibility from the first alternative.

    STATIC_REQUIRE(std::is_default_constructible_v<StatusDefCon>);
    STATIC_REQUIRE_FALSE(std::is_default_constructible_v<StatusNotDefCon>);

}

TEST_CASE("varerr_status_constraints_emplace", "[varerr][status]") {

    constexpr std::size_t kTestIndexBound = 7;

    using R0 = varerr::Row<E<3>, E<5>, E<1>>;
    using RN = varerr::row_normalize_t<UniverseE, R0>;
    using B0 = varerr::status_from_row_t<UniverseE, R0>;

    // The empty status has no in-place constructor.

    STATIC_REQUIRE_FALSE(std::constructible_from<HomStatus<0>, std::in_place_type_t<E<0>>, std::size_t>);

    // The in-place constructor is explicit.

    STATIC_REQUIRE(std::constructible_from<B0, std::in_place_type_t<E<3>>>);
    STATIC_REQUIRE_FALSE(std::is_convertible_v<std::in_place_type_t<E<3>>, B0>);

    // The constructed alternative must be an element of the error row.

    iterate_index_sequence<kTestIndexBound>([]<std::size_t I>(const index_constant<I>) -> void {
        constexpr bool has_alternative = varerr::row_elem_normalized_v<UniverseE, E<I>, RN>;
        STATIC_REQUIRE(std::constructible_from<B0, std::in_place_type_t<E<I>>> == has_alternative);
        STATIC_REQUIRE(std::constructible_from<B0, std::in_place_type_t<E<I>>, std::size_t> == has_alternative);
    });

    // Every cvref-qualification of the argument is accepted (when applicable).

    iterate_cref_matrix<std::size_t>([]<typename T>(std::type_identity<T>) -> void {
        STATIC_REQUIRE(std::constructible_from<B0, std::in_place_type_t<E<3>>, T>);
    });

    iterate_cref_matrix<E<3>>([]<typename T>(std::type_identity<T>) -> void {
        STATIC_REQUIRE(std::constructible_from<B0, std::in_place_type_t<E<3>>, T>);
    });

    // The constructor performs no unexpected conversions (when applicable).

    STATIC_REQUIRE_FALSE(std::constructible_from<B0, std::in_place_type_t<E<3>>, std::size_t*>);
    STATIC_REQUIRE_FALSE(std::constructible_from<B0, std::in_place_type_t<E<3>>, std::size_t, std::size_t>);
    STATIC_REQUIRE_FALSE(std::constructible_from<B0, std::in_place_type_t<E<3>>, std::size_t(*)()>);
    STATIC_REQUIRE_FALSE(std::constructible_from<B0, std::in_place_type_t<E<3>>, std::size_t[]>);
    STATIC_REQUIRE_FALSE(std::constructible_from<B0, std::in_place_type_t<E<3>>, std::size_t[1]>);
    STATIC_REQUIRE_FALSE(std::constructible_from<B0, std::in_place_type_t<E<3>>, std::nullptr_t>);

}

TEST_CASE("varerr_status_constraints_widen", "[varerr][status]") {

    using RowBase = varerr::Row<E<1>, E<3>>;
    using RowExtend = varerr::Row<E<1>, E<2>, E<3>, E<4>>;
    using RowNoExtend = varerr::Row<E<0>, E<2>, E<3>, E<4>>;

    using StatusBase = varerr::status_from_row_t<UniverseE, RowBase>;
    using StatusExtend = varerr::status_from_row_t<UniverseE, RowExtend>;
    using StatusNoExtend = varerr::status_from_row_t<UniverseE, RowNoExtend>;

    // The widening constructor rejects the empty status.

    STATIC_REQUIRE(std::is_constructible_v<HomStatus<2>, HomStatus<1>>);
    STATIC_REQUIRE_FALSE(std::is_constructible_v<HomStatus<2>, HomStatus<0>>);

    // The widening constructor accepts all non-volatile qualifications of an
    // argument that is a non-empty proper subset of the target row.

    iterate_cvref_matrix<StatusBase>([]<typename T>(const std::type_identity<T>) -> void {
        constexpr bool well_formed = !std::is_volatile_v<std::remove_reference_t<T>>;
        STATIC_REQUIRE(std::is_constructible_v<StatusExtend, T> == well_formed);
    });

    // The widening constructor rejects all qualifications of an argument that
    // is not a proper subset of the target row. The equal rows case cannot be
    // tested directly.

    iterate_cvref_matrix<StatusExtend>([]<typename T>(const std::type_identity<T>) -> void {
        STATIC_REQUIRE_FALSE(std::is_constructible_v<StatusBase, T>);
    });

    iterate_cvref_matrix<StatusBase>([]<typename T>(const std::type_identity<T>) -> void {
        STATIC_REQUIRE_FALSE(std::is_constructible_v<StatusNoExtend, T>);
    });

    // The widening constructor is implicit.

    STATIC_REQUIRE(std::is_convertible_v<StatusBase&, StatusExtend>);
    STATIC_REQUIRE(std::is_convertible_v<const StatusBase&, StatusExtend>);

    // The widening constructor is nothrow assignable.

    STATIC_REQUIRE(std::is_assignable_v<StatusExtend&, StatusBase>);
    STATIC_REQUIRE(std::is_nothrow_assignable_v<StatusExtend&, StatusBase>);

    // The source and target universes must agree.

    using UniverseOther = pack_apply_t<bind_adapter<UniverseT>, RowExtend>;
    using StatusOther = varerr::status_from_row_t<UniverseOther, RowExtend>;

    STATIC_REQUIRE(std::is_constructible_v<StatusExtend, StatusBase&>);
    STATIC_REQUIRE(std::is_constructible_v<StatusExtend, const StatusBase&>);
    STATIC_REQUIRE_FALSE(std::is_constructible_v<StatusExtend, StatusOther&>);
    STATIC_REQUIRE_FALSE(std::is_constructible_v<StatusExtend, const StatusOther&>);

}

TEST_CASE("varerr_status_constraints_holds", "[varerr][status]") {

    constexpr std::size_t kTestIndexBound = 7;

    using R0 = varerr::Row<E<1>, E<3>, E<5>>;
    using B0 = varerr::status_from_row_t<UniverseE, R0>;

    // The error row must be non-empty.

    STATIC_REQUIRE_FALSE(IsStatusHoldsWellFormed<HomStatus<0>&, E<0>>);

    // The element must be a member of the error row.

    iterate_index_sequence<kTestIndexBound>([]<std::size_t I>(const index_constant<I>) -> void {
        constexpr bool well_formed = varerr::row_elem_normalized_v<UniverseE, E<I>, R0>;
        STATIC_REQUIRE(IsStatusHoldsWellFormed<B0&, E<I>> == well_formed);
    });

    // Only volatile references are prohibited.

    iterate_cvref_matrix<B0>([]<typename T>(const std::type_identity<T>) -> void {
        constexpr bool well_formed = !std::is_volatile_v<std::remove_reference_t<T>>;
        STATIC_REQUIRE(IsStatusHoldsWellFormed<T, E<1>> == well_formed);
    });

}

TEST_CASE("varerr_status_constraints_index", "[varerr][status]") {

    using R0 = varerr::Row<E<1>, E<3>, E<5>>;
    using B0 = varerr::status_from_row_t<UniverseE, R0>;

    // The error row must be non-empty.

    STATIC_REQUIRE_FALSE(IsStatusIndexWellFormed<HomStatus<0>&>);

    // Only volatile references are prohibited.

    iterate_cvref_matrix<B0>([]<typename T>(const std::type_identity<T>) -> void {
        constexpr bool well_formed = !std::is_volatile_v<std::remove_reference_t<T>>;
        STATIC_REQUIRE(IsStatusIndexWellFormed<T> == well_formed);
    });

}

TEST_CASE("varerr_status_constraints_get_if", "[varerr][status]") {

    constexpr std::size_t kTestIndexBound = 7;

    using R0 = varerr::Row<E<1>, E<3>, E<5>>;
    using B0 = varerr::status_from_row_t<UniverseE, R0>;

    // The error row must be non-empty.

    STATIC_REQUIRE_FALSE(IsStatusGetIfWellFormed<HomStatus<0>&, E<0>>);

    // The element must be a member of the error row.

    iterate_index_sequence<kTestIndexBound>([]<std::size_t I>(const index_constant<I>) -> void {
        constexpr bool well_formed = varerr::row_elem_normalized_v<UniverseE, E<I>, R0>;
        STATIC_REQUIRE(IsStatusGetIfWellFormed<B0&, E<I>> == well_formed);
    });

    // Only non-volatile lvalue references are permitted.

    iterate_cvref_matrix<B0>([]<typename T>(const std::type_identity<T>) -> void {
        constexpr bool well_formed = std::is_lvalue_reference_v<T> && !std::is_volatile_v<std::remove_reference_t<T>>;
        STATIC_REQUIRE(IsStatusGetIfWellFormed<T, E<3>> == well_formed);
    });

    // The returned pointer tracks the constness of the status object.

    iterate_const_matrix<B0>([]<typename T>(const std::type_identity<T>) -> void {
        using GetIfResult = std::conditional_t<std::is_const_v<T>, const E<3>*, E<3>*>;
        STATIC_REQUIRE(std::same_as<decltype(std::declval<T&>().template get_if<E<3>>()), GetIfResult>);
    });

}

TEST_CASE("varerr_status_constraints_get", "[varerr][status]") {

    constexpr std::size_t kTestIndexBound = 7;

    using R0 = varerr::Row<E<1>, E<3>, E<5>>;
    using B0 = varerr::status_from_row_t<UniverseE, R0>;

    // The error row must be non-empty.

    STATIC_REQUIRE_FALSE(IsStatusGetWellFormed<HomStatus<0>&, E<0>>);

    // The element must be a member of the error row.

    iterate_index_sequence<kTestIndexBound>([]<std::size_t I>(const index_constant<I>) -> void {
        constexpr bool well_formed = varerr::row_elem_normalized_v<UniverseE, E<I>, R0>;
        STATIC_REQUIRE(IsStatusGetWellFormed<B0&, E<I>> == well_formed);
    });

    // Only non-volatile lvalue references are permitted.

    iterate_cvref_matrix<B0>([]<typename T>(const std::type_identity<T>) -> void {
        constexpr bool well_formed = std::is_lvalue_reference_v<T> && !std::is_volatile_v<std::remove_reference_t<T>>;
        STATIC_REQUIRE(IsStatusGetWellFormed<T, E<3>> == well_formed);
    });

    // The returned reference tracks the constness of the status object.

    iterate_const_matrix<B0>([]<typename T>(const std::type_identity<T>) -> void {
        using GetResult = std::conditional_t<std::is_const_v<T>, const E<3>&, E<3>&>;
        STATIC_REQUIRE(std::same_as<decltype(std::declval<T&>().template get<E<3>>()), GetResult>);
    });

}

TEST_CASE("varerr_status_constraints_visit", "[varerr][status]") {

    using R0 = varerr::Row<E<1>, E<3>, E<5>>;
    using B0 = varerr::status_from_row_t<UniverseE, R0>;

    // The error row must be non-empty.

    STATIC_REQUIRE_FALSE(IsStatusVisitWellFormed<HomStatus<0>&, VisitorVoidConstL>);

    // Only volatile references are prohibited.

    iterate_cvref_matrix<B0>([]<typename T>(const std::type_identity<T>) -> void {
        constexpr bool well_formed = !std::is_volatile_v<std::remove_reference_t<T>>;
        STATIC_REQUIRE(IsStatusVisitWellFormed<T, VisitorVoidConstL> == well_formed);
    });

    // Non-uniform visitors are rejected.

    STATIC_REQUIRE(IsStatusVisitWellFormed<HomStatus<2>&, VisitorUniformConstL>);
    STATIC_REQUIRE_FALSE(IsStatusVisitWellFormed<HomStatus<2>&, VisitorNonUniformConstL>);

    // Partial visitors are rejected.

    STATIC_REQUIRE(IsStatusVisitWellFormed<HomStatus<1>&, VisitorVoidPartialConstL<0>>);
    STATIC_REQUIRE_FALSE(IsStatusVisitWellFormed<HomStatus<2>&, VisitorVoidPartialConstL<0>>);

    // The value category bound by the visitor is determined by the BasicStatus.

    STATIC_REQUIRE(IsStatusVisitWellFormed<B0&, VisitorVoidR>); /* auto&& */
    STATIC_REQUIRE(IsStatusVisitWellFormed<B0&&, VisitorVoidR>); /* auto&& */
    STATIC_REQUIRE(IsStatusVisitWellFormed<B0&, VisitorVoidL>); /* auto& */
    STATIC_REQUIRE_FALSE(IsStatusVisitWellFormed<B0&&, VisitorVoidL>); /* auto& */

    // References propagate through the visitor dispatch chain.

    iterate_const_matrix<B0>([]<typename T>(const std::type_identity<T>) -> void {
        using VisitInvoke = decltype(std::declval<T&>().visit(VisitorRefPassThruL {}));
        using VisitResult = std::conditional_t<std::is_const_v<T>, const std::size_t&, std::size_t&>;
        STATIC_REQUIRE(std::same_as<VisitInvoke, VisitResult>);
    });

    iterate_const_matrix<B0>([]<typename T>(const std::type_identity<T>) -> void {
        using VisitInvoke = decltype(std::declval<T&>().visit(VisitorRefPassThruConstL {}));
        using VisitResult = const std::size_t&;
        STATIC_REQUIRE(std::same_as<VisitInvoke, VisitResult>);
    });

}

TEST_CASE("varerr_status_constraints_visit_forward", "[varerr][status]") {

    using R0 = varerr::Row<E<1>, E<3>, E<5>>;
    using B0 = varerr::status_from_row_t<UniverseE, R0>;

    // The visit forwards the constness and value category of the visitor.

    STATIC_REQUIRE(IsStatusVisitWellFormed<B0&, VisitorAsLValueVoid&>);
    STATIC_REQUIRE_FALSE(IsStatusVisitWellFormed<B0&, const VisitorAsLValueVoid&>);
    STATIC_REQUIRE_FALSE(IsStatusVisitWellFormed<B0&, VisitorAsLValueVoid&&>);
    STATIC_REQUIRE_FALSE(IsStatusVisitWellFormed<B0&, const VisitorAsLValueVoid&&>);

    STATIC_REQUIRE(IsStatusVisitWellFormed<B0&, VisitorAsConstLValueVoid&>);
    STATIC_REQUIRE(IsStatusVisitWellFormed<B0&, const VisitorAsConstLValueVoid&>);
    STATIC_REQUIRE(IsStatusVisitWellFormed<B0&, VisitorAsConstLValueVoid&&>);
    STATIC_REQUIRE(IsStatusVisitWellFormed<B0&, const VisitorAsConstLValueVoid&&>);

    STATIC_REQUIRE_FALSE(IsStatusVisitWellFormed<B0&, VisitorAsRValueVoid&>);
    STATIC_REQUIRE_FALSE(IsStatusVisitWellFormed<B0&, const VisitorAsRValueVoid&>);
    STATIC_REQUIRE(IsStatusVisitWellFormed<B0&, VisitorAsRValueVoid&&>);
    STATIC_REQUIRE_FALSE(IsStatusVisitWellFormed<B0&, const VisitorAsRValueVoid&&>);

    STATIC_REQUIRE_FALSE(IsStatusVisitWellFormed<B0&, VisitorAsConstRValueVoid&>);
    STATIC_REQUIRE_FALSE(IsStatusVisitWellFormed<B0&, const VisitorAsConstRValueVoid&>);
    STATIC_REQUIRE(IsStatusVisitWellFormed<B0&, VisitorAsConstRValueVoid&&>);
    STATIC_REQUIRE(IsStatusVisitWellFormed<B0&, const VisitorAsConstRValueVoid&&>);

}


TEST_CASE("varerr_status_constraints_exact", "[varerr][status]") {

    using R0 = varerr::Row<E<3>, E<5>, E<1>>;
    using B0 = varerr::status_from_row_t<UniverseE, R0>;

    // The constructors and accessors reject cvref-qualified keys.

    iterate_cvref_matrix<E<3>>([]<typename K>(const std::type_identity<K>) -> void {
        constexpr bool is_exact = std::same_as<K, std::remove_cvref_t<K>>;
        STATIC_REQUIRE(std::constructible_from<B0, std::in_place_type_t<K>, std::size_t> == is_exact);
        STATIC_REQUIRE(IsStatusHoldsWellFormed<B0&, K> == is_exact);
        STATIC_REQUIRE(IsStatusGetWellFormed<B0&, K> == is_exact);
        STATIC_REQUIRE(IsStatusGetIfWellFormed<B0&, K> == is_exact);
    });

}

TEST_CASE("varerr_status_noexcept_default", "[varerr][status]") {

    using RowDefCon = varerr::Row<E<0>, DefaultThrowType>;
    using RowThrowDefCon = varerr::Row<DefaultThrowType, E<0>>;

    struct UniDefCon : pack_apply_t<bind_adapter<UniverseT>, RowDefCon> {};
    struct UniThrowDefCon : pack_apply_t<bind_adapter<UniverseT>, RowThrowDefCon> {};

    using StatusDefCon = varerr::status_from_row_t<UniDefCon, RowDefCon>;
    using StatusThrowDefCon = varerr::status_from_row_t<UniThrowDefCon, RowThrowDefCon>;

    // BasicStatus inherits nothrow default constructibility from its first al-
    // ternative.

    STATIC_REQUIRE(std::is_nothrow_default_constructible_v<StatusDefCon>);
    STATIC_REQUIRE_FALSE(std::is_nothrow_default_constructible_v<StatusThrowDefCon>);

}

TEST_CASE("varerr_status_noexcept_emplace", "[varerr][status]") {

    using R0 = varerr::Row<E<0>, ConditionalThrowType, E<1>>;
    using U0 = pack_apply_t<bind_adapter<UniverseT>, R0>;
    using B0 = varerr::status_from_row_t<U0, R0>;

    // The in-place constructor inherits nothrow constructibility from the al-
    // ternative.

    STATIC_REQUIRE(std::is_nothrow_constructible_v<B0, std::in_place_type_t<ConditionalThrowType>, int>);
    STATIC_REQUIRE_FALSE(std::is_nothrow_constructible_v<B0, std::in_place_type_t<ConditionalThrowType>, double>);

}

TEST_CASE("varerr_status_noexcept_widen", "[varerr][status]") {

    using R1 = varerr::Row<E<0>, ConditionalThrowType, E<1>>;
    using U1 = pack_apply_t<bind_adapter<UniverseT>, R1>;
    using B1 = varerr::status_from_row_t<U1, R1>;

    using R0 = varerr::Row<ConditionalThrowType>;
    using B0 = varerr::status_from_row_t<U1, R0>;

    // The widening constructor is unconditionally noexcept. The error row con-
    // tains an alternative with a throwing constructor. The widening construc-
    // tor copies the active alternative and is thus unconditionally noexcept.

    STATIC_REQUIRE_FALSE(std::is_nothrow_constructible_v<B1, std::in_place_type_t<ConditionalThrowType>, double>);
    STATIC_REQUIRE(std::is_nothrow_constructible_v<B1, B0>);

}

TEST_CASE("varerr_status_noexcept_holds", "[varerr][status]") {

    constexpr std::size_t kTestBound = 3;

    iterate_index_sequence<kTestBound>([]<std::size_t I>(const index_constant<I>) -> void {
        if constexpr (I > 0) {
            iterate_const_matrix<HomStatus<I>>([]<typename S>(const std::type_identity<S>) -> void {
                iterate_index_sequence<I>([]<std::size_t J>(const index_constant<J>) -> void {
                    STATIC_REQUIRE(noexcept(std::declval<S&>().template holds<E<J>>()));
                });
            });
        }
    });

}

TEST_CASE("varerr_status_noexcept_index", "[varerr][status]") {

    constexpr std::size_t kTestBound = 3;

    iterate_index_sequence<kTestBound>([]<std::size_t I>(const index_constant<I>) -> void {
        if constexpr (I > 0) {
            iterate_const_matrix<HomStatus<I>>([]<typename S>(const std::type_identity<S>) -> void {
                STATIC_REQUIRE(noexcept(std::declval<S&>().index()));
            });
        }
    });

}

TEST_CASE("varerr_status_noexcept_get_if", "[varerr][status]") {

    constexpr std::size_t kTestBound = 3;

    iterate_index_sequence<kTestBound>([]<std::size_t I>(const index_constant<I>) -> void {
        if constexpr (I > 0) {
            iterate_const_matrix<HomStatus<I>>([]<typename S>(const std::type_identity<S>) -> void {
                iterate_index_sequence<I>([]<std::size_t J>(const index_constant<J>) -> void {
                    STATIC_REQUIRE(noexcept(std::declval<S&>().template get_if<E<J>>()));
                });
            });
        }
    });

}

TEST_CASE("varerr_status_noexcept_get", "[varerr][status]") {

    constexpr std::size_t kTestBound = 3;

    iterate_index_sequence<kTestBound>([]<std::size_t I>(const index_constant<I>) -> void {
        if constexpr (I > 0) {
            iterate_const_matrix<HomStatus<I>>([]<typename S>(const std::type_identity<S>) -> void {
                iterate_index_sequence<I>([]<std::size_t J>(const index_constant<J>) -> void {
                    STATIC_REQUIRE(noexcept(std::declval<S&>().template get<E<J>>()));
                });
            });
        }
    });

}

TEST_CASE("varerr_status_noexcept_visit", "[varerr][status]") {

    constexpr std::size_t kIndexTestBound = 7;

    using R0 = varerr::Row<E<1>, E<3>, E<5>>;
    using B0 = varerr::status_from_row_t<UniverseE, R0>;

    // The visit is noexcept if the visitor is noexcept for every alternative.

    iterate_index_sequence<kIndexTestBound>([]<std::size_t I>(const index_constant<I>) -> void {
        constexpr bool is_noexcept = noexcept(std::declval<B0&>().visit(VisitorThrowOnIndexConstL<I> {}));
        constexpr bool is_alternative = varerr::row_elem_normalized_v<UniverseE, E<I>, R0>;
        STATIC_REQUIRE(is_noexcept == !is_alternative);
    });

    // The noexcept specification is sensitive to constness.

    iterate_cref_matrix<B0>([]<typename T>(std::type_identity<T>) -> void {
        constexpr bool is_noexcept = noexcept(std::declval<T>().visit(VisitorThrowOnConstL {}));
        constexpr bool is_throwing = std::is_lvalue_reference_v<T> && std::is_const_v<std::remove_reference_t<T>>;
        STATIC_REQUIRE(is_noexcept == !is_throwing);
    });

    // The noexcept specification is sensitive to value category. Note that T&&
    // is required because both T and T&& bind to the rvalue overload but plain
    // T is not an rvalue reference (i.e., std::is_rvalue_reference_v is false).

    iterate_cref_matrix<B0>([]<typename T>(std::type_identity<T>) -> void {
        constexpr bool is_noexcept = noexcept(std::declval<T>().visit(VisitorThrowOnR {}));
        constexpr bool is_throwing = std::is_rvalue_reference_v<T&&> && !std::is_const_v<std::remove_reference_t<T>>;
        STATIC_REQUIRE(is_noexcept == !is_throwing);
    });

}
