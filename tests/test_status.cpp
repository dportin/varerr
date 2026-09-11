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

} // namespace

TEMPLATE_TEST_CASE("varerr_status_trivial", "[varerr][status]",
    HomStatus<0>, HomStatus<3>, (HetStatus<0, 3>), (HetStatus<3, 3>)
) {

    using TrivialStatus = varerr::Status<UniverseI, TrivialType>;

    // BasicStatus inherits triviality from Storage.

    STATIC_REQUIRE(std::is_trivially_copyable_v<TestType>);
    STATIC_REQUIRE(std::is_trivially_destructible_v<TestType>);
    STATIC_REQUIRE(std::is_trivially_copy_constructible_v<TestType>);
    STATIC_REQUIRE(std::is_trivially_move_constructible_v<TestType>);
    STATIC_REQUIRE(std::is_trivially_copy_assignable_v<TestType>);
    STATIC_REQUIRE(std::is_trivially_move_assignable_v<TestType>);

    // BasicStatus is never trivially default constructible.

    STATIC_REQUIRE_FALSE(std::is_trivially_default_constructible_v<HomStatus<1>>);
    STATIC_REQUIRE_FALSE(std::is_trivially_default_constructible_v<TrivialStatus>);

}

TEST_CASE("varerr_status_construct_empty", "[varerr][status]") {

    using NoDefaultStatus = varerr::Status<UniverseI, NoDefaultConstructType>;

    // The empty BasicStatus is never constructible.

    STATIC_REQUIRE_FALSE(std::is_default_constructible_v<HomStatus<0>>);

    // A non-empty BasicStatus inherits constructibility from its alternatives.

    STATIC_REQUIRE(std::is_default_constructible_v<HomStatus<1>>);
    STATIC_REQUIRE_FALSE(std::is_default_constructible_v<NoDefaultStatus>);

}

TEST_CASE("varerr_status_construct_default", "[varerr][status]") {

    using R0 = varerr::Row<E<3>, E<1>>;
    using B0 = varerr::status_from_row_t<UniverseE, R0>;

    // The default constructor default-constructs the first alternative.

    constexpr B0 status {};

    STATIC_REQUIRE(status.holds<E<1>>());
    STATIC_REQUIRE(status.get<E<1>>().value() == 0);
    STATIC_REQUIRE(status.index() == 0);

}

TEST_CASE("varerr_status_construct_emplace", "[varerr][status]") {
    REQUIRE(false);
}

TEST_CASE("varerr_status_construct_widen", "[varerr][status]") {
    REQUIRE(false);
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
    REQUIRE(false);
}

TEST_CASE("varerr_status_constraints_widen", "[varerr][status]") {
    REQUIRE(false);
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
    REQUIRE(false);
}

TEST_CASE("varerr_status_noexcept_widen", "[varerr][status]") {
    REQUIRE(false);
}

TEST_CASE("varerr_status_noexcept_holds", "[varerr][status]") {

    constexpr std::size_t kTestBound = 3;

    iterate_index_sequence<kTestBound>([]<std::size_t I>(const index_constant<I>) -> void {
        iterate_index_sequence<I>([]<std::size_t J>(const index_constant<J>) -> void {
            if constexpr (I > 0) {
                STATIC_REQUIRE(noexcept(std::declval<HomStatus<I>&>().template holds<E<J>>()));
            }
        });
    });

}

TEST_CASE("varerr_status_noexcept_index", "[varerr][status]") {

    constexpr std::size_t kTestBound = 3;

    iterate_index_sequence<kTestBound>([]<std::size_t I>(const index_constant<I>) -> void {
        if constexpr (I > 0) {
            STATIC_REQUIRE(noexcept(std::declval<HomStatus<I>&>().index()));
        }
    });

}

TEST_CASE("varerr_status_noexcept_get_if", "[varerr][status]") {

    constexpr std::size_t kTestBound = 3;

    iterate_index_sequence<kTestBound>([]<std::size_t I>(const index_constant<I>) -> void {
        iterate_index_sequence<I>([]<std::size_t J>(const index_constant<J>) -> void {
            if constexpr (I > 0) {
                STATIC_REQUIRE(noexcept(std::declval<HomStatus<I>&>().template get_if<E<J>>()));
            }
        });
    });

}

TEST_CASE("varerr_status_noexcept_get", "[varerr][status]") {

    constexpr std::size_t kTestBound = 3;

    iterate_index_sequence<kTestBound>([]<std::size_t I>(const index_constant<I>) -> void {
        iterate_index_sequence<I>([]<std::size_t J>(const index_constant<J>) -> void {
            if constexpr (I > 0) {
                STATIC_REQUIRE(noexcept(std::declval<HomStatus<I>&>().template get<E<J>>()));
            }
        });
    });

}

TEST_CASE("varerr_status_noexcept_visit", "[varerr][status]") {

    constexpr std::size_t kIndexTestBound = 7;

    using R0 = varerr::Row<E<1>, E<3>, E<5>>;
    using B0 = varerr::status_from_row_t<UniverseE, R0>;

    // The visit is noexcept if the visitor is noexcept for every alternative.

    iterate_index_sequence<kIndexTestBound>([]<std::size_t I>(index_constant<I>) -> void {
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
