#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>

#include "include/utilities.hpp"
#include "include/universe.hpp"

#include <varerr/algebra.hpp>

#include <array>
#include <bit>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <optional>
#include <type_traits>
#include <utility>

using namespace varerr::tests;
using namespace varerr::tests::universe;

namespace {

// Exhaustively test compile-time ordered sets using k-element bitsets as refer-
// ence implementation. Increasing the universe size exponentially scales compi-
// le times. The following universes cover all relevant cases.

constexpr std::size_t kAlgebraMaxUnaryElems = 4;
constexpr std::size_t kAlgebraMaxUnaryMask = std::size_t {1} << kAlgebraMaxUnaryElems;

constexpr std::size_t kAlgebraMaxBinaryElems = 4;
constexpr std::size_t kAlgebraMaxBinaryMask = std::size_t {1} << kAlgebraMaxBinaryElems;

constexpr std::size_t kAlgebraMaxTernaryElems = 3;
constexpr std::size_t kAlgebraMaxTernaryMask = std::size_t {1} << kAlgebraMaxTernaryElems;

// Lift the homogeneous test universe to a Row.

template <std::size_t N>
using L = lift_index_sequence_t<varerr::Row, E, N>;

static_assert(std::same_as<L<0>, varerr::Row<>>);
static_assert(std::same_as<L<1>, varerr::Row<E<0>>>);
static_assert(std::same_as<L<2>, varerr::Row<E<0>,E<1>>>);

// Lift the homogeneous const universe to a Row.

template <std::size_t N>
using C = lift_index_sequence_t<varerr::Row, ConstUniverseE::unrank, N>;

static_assert(std::same_as<C<0>, varerr::Row<>>);
static_assert(std::same_as<C<1>, varerr::Row<E<0>>>);
static_assert(std::same_as<C<2>, varerr::Row<E<0>, const E<0>>>);
static_assert(std::same_as<C<3>, varerr::Row<E<0>, const E<0>, E<1>>>);

// Determine whether a row operation is well-formed.

template <typename M, typename... Us>
concept IsRowUnionNormalizedWellFormed = requires {
    typename varerr::row_union_normalized_t<M, Us...>;
};

template <typename M, typename A, typename U>
concept IsRowLookupNormalizedWellFormed = requires {
    varerr::row_lookup_normalized_v<M, A, U>;
};

// Prepend an element to a Row.

template <typename F, typename R>
struct row_cons;

template <typename F, typename... Fs>
struct row_cons<F, varerr::Row<Fs...>> : std::type_identity<varerr::Row<F, Fs...>> {};

template <typename F, typename Redex>
using row_cons_adapter = row_cons<F, typename Redex::type>;

// Convert a bitmask to a Row of E<I> elements.

template <std::size_t Mask, typename Indices>
struct mask_to_row;

template <std::size_t Mask>
struct mask_to_row<Mask, std::index_sequence<>> : std::type_identity<varerr::Row<>> {};

template <std::size_t Mask, std::size_t I, std::size_t... Is>
struct mask_to_row<Mask, std::index_sequence<I, Is...>> : std::conditional_t<
    (Mask & (std::size_t {1} << I)) != 0,
    row_cons_adapter<E<I>, mask_to_row<Mask, std::index_sequence<Is...>>>,
    mask_to_row<Mask, std::index_sequence<Is...>>
> {};

template <std::size_t Mask>
using mask_to_row_t = mask_to_row<Mask, std::make_index_sequence<std::bit_width(Mask)>>::type;

// Naive computation of the Nth set bit in a bitmask.

[[nodiscard]] constexpr std::size_t nth_set_bit(std::size_t mask, std::size_t n) noexcept {

    assert(mask != 0);

    while (n--) {
        mask &= mask - 1;
    }

    std::size_t index = 0;
    while ((mask & 1) == 0) {
        mask >>= 1;
        ++index;
    }

    return index;

}

// Naive computation of n!.

[[nodiscard]] constexpr std::size_t factorial(std::size_t n) noexcept {

    std::size_t result = 1;
    for (std::size_t i = 2; i <= n; ++i) {
        result *= i;
    }

    return result;

}

// Exhaustively test permutations of a k-element set corresponding to a given
// mask by enumerating Lehmer codes corresponding to the set.

template <std::size_t Code, std::size_t Mask>
[[nodiscard]] consteval auto lehmer_code_to_row_impl() {

    constexpr std::size_t mask_bits = static_cast<std::size_t>(std::popcount(Mask));

    // [code] is the current Lehmer code; [size] the size of the current permut-
    // ation (one more than the current factorial base); and [mask] the bitmask
    // of remaining elements in the universe.

    std::size_t code = Code;
    std::size_t size = mask_bits;
    std::size_t mask = Mask;

    std::array<std::size_t, mask_bits> indices {};

    while (size--) {

        // The quotient is the coefficient of the current factorial base, which
        // counts the number of remaining elements in the universe smaller than
        // the decoded element (the number of inversions in the reduced univer-
        // se). The decoded element is thus the (coefficient + 1)th set bit in
        // the current mask.

        const std::size_t base = factorial(size);
        const std::size_t inversions = code / base;
        code %= base;

        const std::size_t elem = nth_set_bit(mask, inversions);
        mask &= ~(std::size_t {1} << elem);
        indices[size] = elem;

    }

    return indices;

}

template <std::size_t Code, std::size_t Mask>
struct lehmer_code_to_row {

    static constexpr auto perm_indices = lehmer_code_to_row_impl<Code, Mask>();
    static constexpr auto perm_indices_size = perm_indices.size();

    using type = decltype(
        []<std::size_t... Is>(std::index_sequence<Is...>) -> varerr::Row<E<perm_indices[Is]>...> {
            return {};
        }(std::make_index_sequence<perm_indices_size> {})
    );

};

template <std::size_t Code, std::size_t Mask>
using lehmer_code_to_row_t = lehmer_code_to_row<Code, Mask>::type;

// Build unary compile-time comparison table between row and bitset operations.

template <std::size_t D0, typename Pred>
consteval auto build_compare_table1(Pred pred) {

    std::array<bool, D0> table {};

    iterate_index_sequence<D0>([&](const auto x) {
        constexpr std::size_t X = decltype(x)::value;
        table[X] = pred(x);
    });

    return table;

}

template <std::size_t D0, typename Pred>
void row_algebra_compare_test1(Pred pred) {

    constexpr auto table = build_compare_table1<D0>(pred);

    for (std::size_t x = 0; x < D0; ++x) {
        CAPTURE(x);
        REQUIRE(table[x]);
    }

}

// Build binary compile-time comparison table between row and bitset operations.

template <std::size_t D0, std::size_t D1, typename Pred>
consteval auto build_compare_table2(Pred pred) {

    std::array<bool, D0 * D1> table {};

    iterate_index_sequence<D0>([&](const auto x) {
        constexpr std::size_t X = decltype(x)::value;
        iterate_index_sequence<D1>([&](const auto y) {
            constexpr std::size_t Y = decltype(y)::value;
            table[X * D1 + Y] = pred(x, y);
        });
    });

    return table;

}

template <std::size_t D0, std::size_t D1, typename Pred>
void row_algebra_compare_test2(Pred pred) {

    constexpr auto table = build_compare_table2<D0, D1>(pred);

    for (std::size_t x = 0; x < D0; ++x) {
        for (std::size_t y = 0; y < D1; ++y) {
            CAPTURE(x, y);
            REQUIRE(table[x * D1 + y]);
        }
    }

}

// Build ternary compile-time comparison table between row and bitset operations.

template <std::size_t D0, std::size_t D1, std::size_t D2, typename Pred>
consteval auto build_compare_table3(Pred pred) {

    std::array<bool, D0 * D1 * D2> table {};

    iterate_index_sequence<D0>([&](const auto x) {
        constexpr std::size_t X = decltype(x)::value;
        iterate_index_sequence<D1>([&](const auto y) {
            constexpr std::size_t Y = decltype(y)::value;
            iterate_index_sequence<D2>([&](const auto z) {
                constexpr std::size_t Z = decltype(z)::value;
                table[(X * D1 + Y) * D2 + Z] = pred(x, y, z);
            });
        });
    });

    return table;

}

template <std::size_t D0, std::size_t D1, std::size_t D2, typename Pred>
void row_algebra_compare_test3(Pred pred) {

    constexpr auto table = build_compare_table3<D0, D1, D2>(pred);

    for (std::size_t x = 0; x < D0; ++x) {
        for (std::size_t y = 0; y < D1; ++y) {
            for (std::size_t z = 0; z < D2; ++z) {
                CAPTURE(x, y, z);
                REQUIRE(table[(x * D1 + y) * D2 + z]);
            }
        }
    }

}

} // namespace

TEST_CASE("varerr_algebra_traits_is_ranked", "[varerr][algebra]") {

    iterate_cvref_matrix<E<0>>([]<typename T>(const std::type_identity<T>) -> void {
        STATIC_REQUIRE(varerr::IsRanked<UniverseE, T>);
    });

    iterate_cvref_matrix<H<8,3>>([]<typename T>(const std::type_identity<T>) -> void {
        STATIC_REQUIRE(varerr::IsRanked<UniverseH, T>);
    });

    STATIC_REQUIRE_FALSE(varerr::IsRanked<UniverseH, E<0>>);
    STATIC_REQUIRE_FALSE(varerr::IsRanked<UniverseE, H<8,3>>);
    STATIC_REQUIRE_FALSE(varerr::IsRanked<UniverseE, varerr::Row<>>);
    STATIC_REQUIRE_FALSE(varerr::IsRanked<UniverseE, varerr::Row<E<0>>>);

}

TEST_CASE("varerr_algebra_traits_is_row", "[varerr][algebra]") {

    constexpr std::size_t kAlgebraTraitsIsRowBound = 10;

    iterate_index_sequence<kAlgebraTraitsIsRowBound>([](const auto index) -> void {
        constexpr std::size_t I = decltype(index)::value;
        iterate_cvref_matrix<L<I>>([]<typename T>(const std::type_identity<T>) -> void {
            STATIC_REQUIRE(varerr::IsRow<T>);
        });
    });

    STATIC_REQUIRE_FALSE(varerr::IsRow<int>);
    STATIC_REQUIRE_FALSE(varerr::IsRow<E<0>>);

}

TEST_CASE("varerr_algebra_traits_is_ranked_row", "[varerr][algebra]") {

    constexpr std::size_t kAlgebraTraitsIsRankedRowBound = 10;

    iterate_index_sequence<kAlgebraTraitsIsRankedRowBound>([](const auto index) -> void {
        constexpr std::size_t I = decltype(index)::value;
        iterate_cvref_matrix<L<I>>([]<typename T>(const std::type_identity<T>) -> void {
            STATIC_REQUIRE(varerr::IsRankedRow<UniverseE, T>);
        });
    });

    STATIC_REQUIRE_FALSE(varerr::IsRankedRow<UniverseE, int>);
    STATIC_REQUIRE_FALSE(varerr::IsRankedRow<UniverseH, L<2>>);
    STATIC_REQUIRE_FALSE(varerr::IsRankedRow<UniverseE, varerr::Row<E<0>, int, E<1>>>);

}

TEST_CASE("varerr_algebra_traits_row_size", "[varerr][algebra]") {

    constexpr std::size_t kAlgebraTraitsRowSizeBound = 10;

    iterate_index_sequence<kAlgebraTraitsRowSizeBound>([](const auto index) -> void {
        constexpr std::size_t I = decltype(index)::value;
        iterate_cvref_matrix<L<I>>([]<typename T>(const std::type_identity<T>) -> void {
            STATIC_REQUIRE(varerr::row_size_v<T> == I);
        });
    });

}

TEST_CASE("varerr_algebra_traits_is_normalized_row", "[varerr][algebra]") {

    iterate_index_sequence<kAlgebraMaxBinaryMask>([](const auto mask) -> void {

        constexpr std::size_t Mask = decltype(mask)::value;
        constexpr std::size_t MaskElems = std::popcount(Mask);
        constexpr std::size_t MaskPerms = factorial(MaskElems);

        row_algebra_compare_test1<MaskPerms>([](const auto code) consteval -> bool {
            constexpr std::size_t Code = decltype(code)::value;
            using Row = mask_to_row_t<Mask>;
            using RowPerm = lehmer_code_to_row_t<Code, Mask>;
            return varerr::IsNormalizedRow<UniverseE, RowPerm> == std::same_as<Row, RowPerm>;
        });

    });

    iterate_cvref_matrix<L<5>>([]<typename T>(const std::type_identity<T>) -> void {
        STATIC_REQUIRE(varerr::IsNormalizedRow<UniverseE, T>);
    });

}

// TODO: Test uniqueness and de-duplication.

TEST_CASE("varerr_algebra_normalize_permute", "[varerr][algebra]") {

    iterate_index_sequence<kAlgebraMaxBinaryMask>([](const auto mask) -> void {

        constexpr std::size_t Mask = decltype(mask)::value;
        constexpr std::size_t MaskElems = std::popcount(Mask);
        constexpr std::size_t MaskPerms = factorial(MaskElems);

        row_algebra_compare_test1<MaskPerms>([](const auto code) consteval -> bool {
            constexpr std::size_t Code = decltype(code)::value;
            return std::same_as<varerr::row_normalize_t<UniverseE, lehmer_code_to_row_t<Code, Mask>>, mask_to_row_t<Mask>>;
        });

    });

    iterate_cvref_matrix<L<2>>([]<typename T>(const std::type_identity<T>) -> void {
        STATIC_REQUIRE(std::same_as<varerr::row_normalize_t<UniverseE, T>, varerr::Row<E<0>,E<1>>>);
    });

}

TEST_CASE("varerr_algebra_lookup_normalized", "[varerr][algebra]") {

    row_algebra_compare_test2<kAlgebraMaxBinaryElems, kAlgebraMaxBinaryMask>([](const auto x, const auto y) consteval -> bool {

        constexpr std::size_t X = decltype(x)::value;
        constexpr std::size_t Y = decltype(y)::value;

        constexpr auto result = varerr::row_lookup_normalized_v<UniverseE, E<X>, mask_to_row_t<Y>>;
        constexpr auto present = static_cast<bool>((Y >> X) & std::size_t {1});
        constexpr auto position = static_cast<std::size_t>(std::popcount(Y & ((std::size_t {1} << X) - 1)));

        return present ? result.has_value() && result.value() == position : !result.has_value();

    });

    iterate_cvref_matrix<E<2>>([]<typename A>(const std::type_identity<A>) -> void {
        iterate_cvref_matrix<L<5>>([]<typename U>(const std::type_identity<U>) -> void {
            STATIC_REQUIRE(varerr::row_lookup_normalized_v<UniverseE, A, U>.value() == 2);
            STATIC_REQUIRE(varerr::row_elem_normalized_v<UniverseE, A, U>);
            STATIC_REQUIRE(varerr::row_index_normalized_v<UniverseE, A, U> == 2);
        });
    });


}

TEST_CASE("varerr_algebra_subset_normalized", "[varerr][algebra]") {

    row_algebra_compare_test2<kAlgebraMaxBinaryMask, kAlgebraMaxBinaryMask>([](const auto x, const auto y) consteval -> bool {
        constexpr std::size_t X = decltype(x)::value;
        constexpr std::size_t Y = decltype(y)::value;
        return varerr::row_subset_normalized_v<UniverseE, mask_to_row_t<X>, mask_to_row_t<Y>> == ((X & ~Y) == 0);
    });

    iterate_cvref_matrix<L<1>>([]<typename U>(const std::type_identity<U>) -> void {
        iterate_cvref_matrix<L<3>>([]<typename V>(const std::type_identity<V>) -> void {
            STATIC_REQUIRE(varerr::row_subset_normalized_v<UniverseE, U, V>);
        });
    });

}

TEST_CASE("varerr_algebra_proper_subset_normalized", "[varerr][algebra]") {

    row_algebra_compare_test2<kAlgebraMaxBinaryMask, kAlgebraMaxBinaryMask>([](const auto x, const auto y) consteval -> bool {
        constexpr std::size_t X = decltype(x)::value;
        constexpr std::size_t Y = decltype(y)::value;
        return varerr::row_proper_subset_normalized_v<UniverseE, mask_to_row_t<X>, mask_to_row_t<Y>> == (((X & ~Y) == 0) && (X != Y));
    });

    iterate_cvref_matrix<L<1>>([]<typename U>(const std::type_identity<U>) -> void {
        iterate_cvref_matrix<L<3>>([]<typename V>(const std::type_identity<V>) -> void {
            STATIC_REQUIRE(varerr::row_proper_subset_normalized_v<UniverseE, U, V>);
        });
    });

}

TEST_CASE("varerr_algebra_equiv_normalized", "[varerr][algebra]") {

    row_algebra_compare_test2<kAlgebraMaxBinaryMask, kAlgebraMaxBinaryMask>([](const auto x, const auto y) consteval -> bool {
        constexpr std::size_t X = decltype(x)::value;
        constexpr std::size_t Y = decltype(y)::value;
        return varerr::row_equiv_normalized_v<UniverseE, mask_to_row_t<X>, mask_to_row_t<Y>> == (X == Y);
    });

    iterate_cvref_matrix<L<3>>([]<typename U>(const std::type_identity<U>) -> void {
        iterate_cvref_matrix<L<3>>([]<typename V>(const std::type_identity<V>) -> void {
            STATIC_REQUIRE(varerr::row_equiv_normalized_v<UniverseE, U, V>);
        });
    });

}

// TODO: Test densely-ranked rows.

TEST_CASE("varerr_algebra_union_normalized", "[varerr][algebra]") {

    row_algebra_compare_test2<kAlgebraMaxBinaryMask, kAlgebraMaxBinaryMask>([](const auto x, const auto y) consteval -> bool {
        constexpr std::size_t X = decltype(x)::value;
        constexpr std::size_t Y = decltype(y)::value;
        return std::same_as<varerr::row_union_normalized_t<UniverseE, mask_to_row_t<X>, mask_to_row_t<Y>>, mask_to_row_t<X | Y>>;
    });

    iterate_cvref_matrix<varerr::Row<E<0>,E<1>>>([]<typename U>(const std::type_identity<U>) -> void {
        iterate_cvref_matrix<varerr::Row<E<1>,E<2>>>([]<typename V>(const std::type_identity<V>) -> void {
            STATIC_REQUIRE(std::same_as<varerr::row_union_normalized_t<UniverseE, U, V>, L<3>>);
        });
    });

}

TEST_CASE("varerr_algebra_union_normalized_ternary", "[varerr][algebra]") {

    row_algebra_compare_test3<kAlgebraMaxTernaryMask, kAlgebraMaxTernaryMask, kAlgebraMaxTernaryMask>([](const auto x, const auto y, const auto z) consteval -> bool {
        constexpr std::size_t X = decltype(x)::value;
        constexpr std::size_t Y = decltype(y)::value;
        constexpr std::size_t Z = decltype(z)::value;
        return std::same_as<
            varerr::row_union_normalized_t<UniverseE, mask_to_row_t<X>, mask_to_row_t<Y>, mask_to_row_t<Z>>,
            mask_to_row_t<(X | Y) | Z>
        >;
    });

}

TEST_CASE("varerr_algebra_union_normalized_unary", "[varerr][algebra]") {

    row_algebra_compare_test1<kAlgebraMaxUnaryMask>([](const auto x) consteval -> bool {
        constexpr std::size_t X = decltype(x)::value;
        return std::same_as<varerr::row_union_normalized_t<UniverseE, mask_to_row_t<X>>, mask_to_row_t<X>>;
    });

}

TEST_CASE("varerr_algebra_intersection_normalized", "[varerr][algebra]") {

    row_algebra_compare_test2<kAlgebraMaxBinaryMask, kAlgebraMaxBinaryMask>([](const auto x, const auto y) consteval -> bool {
        constexpr std::size_t X = decltype(x)::value;
        constexpr std::size_t Y = decltype(y)::value;
        return std::same_as<varerr::row_intersection_normalized_t<UniverseE, mask_to_row_t<X>, mask_to_row_t<Y>>, mask_to_row_t<X & Y>>;
    });

    iterate_cvref_matrix<varerr::Row<E<0>,E<1>>>([]<typename U>(const std::type_identity<U>) -> void {
        iterate_cvref_matrix<varerr::Row<E<1>,E<2>>>([]<typename V>(const std::type_identity<V>) -> void {
            STATIC_REQUIRE(std::same_as<varerr::row_intersection_normalized_t<UniverseE, U, V>, varerr::Row<E<1>>>);
        });
    });

}

TEST_CASE("varerr_algebra_intersection_normalized_ternary", "[varerr][algebra]") {

    row_algebra_compare_test3<kAlgebraMaxTernaryMask, kAlgebraMaxTernaryMask, kAlgebraMaxTernaryMask>([](const auto x, const auto y, const auto z) consteval -> bool {
        constexpr std::size_t X = decltype(x)::value;
        constexpr std::size_t Y = decltype(y)::value;
        constexpr std::size_t Z = decltype(z)::value;
        return std::same_as<
            varerr::row_intersection_normalized_t<UniverseE, mask_to_row_t<X>, mask_to_row_t<Y>, mask_to_row_t<Z>>,
            mask_to_row_t<(X & Y) & Z>
        >;
    });

}

TEST_CASE("varerr_algebra_intersection_normalized_unary", "[varerr][algebra]") {

    row_algebra_compare_test1<kAlgebraMaxUnaryMask>(
        [](const auto x) consteval -> bool {
            constexpr std::size_t X = decltype(x)::value;
            return std::same_as<varerr::row_intersection_normalized_t<UniverseE, mask_to_row_t<X>>, mask_to_row_t<X>>;
        });

}

TEST_CASE("varerr_algebra_difference_normalized", "[varerr][algebra]") {

    row_algebra_compare_test2<kAlgebraMaxBinaryMask, kAlgebraMaxBinaryMask>([](const auto x, const auto y) consteval -> bool {
        constexpr std::size_t X = decltype(x)::value;
        constexpr std::size_t Y = decltype(y)::value;
        return std::same_as<varerr::row_difference_normalized_t<UniverseE, mask_to_row_t<X>, mask_to_row_t<Y>>, mask_to_row_t<X & ~Y>>;
    });

    iterate_cvref_matrix<varerr::Row<E<0>,E<1>,E<2>>>([]<typename U>(const std::type_identity<U>) -> void {
        iterate_cvref_matrix<varerr::Row<E<0>,E<2>>>([]<typename V>(const std::type_identity<V>) -> void {
            STATIC_REQUIRE(std::same_as<varerr::row_difference_normalized_t<UniverseE, U, V>, varerr::Row<E<1>>>);
        });
    });

}

TEST_CASE("varerr_algebra_difference_normalized_ternary", "[varerr][algebra]") {

    row_algebra_compare_test3<kAlgebraMaxTernaryMask, kAlgebraMaxTernaryMask, kAlgebraMaxTernaryMask>([](const auto x, const auto y, const auto z) consteval -> bool {
        constexpr std::size_t X = decltype(x)::value;
        constexpr std::size_t Y = decltype(y)::value;
        constexpr std::size_t Z = decltype(z)::value;
        return std::same_as<
            varerr::row_difference_normalized_t<UniverseE, mask_to_row_t<X>, mask_to_row_t<Y>, mask_to_row_t<Z>>,
            mask_to_row_t<X & ~(Y | Z)>
        >;
    });

}

TEST_CASE("varerr_algebra_difference_normalized_unary", "[varerr][algebra]") {

    row_algebra_compare_test1<kAlgebraMaxUnaryMask>([](const auto x) consteval -> bool {
        constexpr std::size_t X = decltype(x)::value;
        return std::same_as<varerr::row_difference_normalized_t<UniverseE, mask_to_row_t<X>>, mask_to_row_t<X>>;
    });

}

TEST_CASE("varerr_algebra_qualified_traits_unit", "[varerr][algebra]") {

    using C6NoE1 = varerr::Row<E<0>, const E<0>, const E<1>, E<2>, const E<2>>;
    using C6NoConstE1 = varerr::Row<E<0>, const E<0>, E<1>, E<2>, const E<2>>;

    STATIC_REQUIRE(varerr::IsNormalizedRow<ConstUniverseE, varerr::Row<E<0>, const E<0>>>);
    STATIC_REQUIRE_FALSE(varerr::IsNormalizedRow<ConstUniverseE, varerr::Row<const E<0>, E<0>>>);

    STATIC_REQUIRE(varerr::row_lookup_normalized_v<ConstUniverseE, E<1>, C<6>>.value_or(42) == 2);
    STATIC_REQUIRE(varerr::row_lookup_normalized_v<ConstUniverseE, const E<1>, C<6>>.value_or(42) == 3);
    STATIC_REQUIRE_FALSE(varerr::row_lookup_normalized_v<ConstUniverseE, E<1>, C6NoE1>.has_value());
    STATIC_REQUIRE_FALSE(varerr::row_lookup_normalized_v<ConstUniverseE, const E<1>, C6NoConstE1>.has_value());

    STATIC_REQUIRE(varerr::row_subset_normalized_v<ConstUniverseE, C6NoE1, C<6>>);
    STATIC_REQUIRE(varerr::row_subset_normalized_v<ConstUniverseE, C6NoConstE1, C<6>>);
    STATIC_REQUIRE_FALSE(varerr::row_subset_normalized_v<ConstUniverseE, varerr::Row<const E<1>>, varerr::Row<E<0>, E<1>, E<2>>>);
    STATIC_REQUIRE_FALSE(varerr::row_subset_normalized_v<ConstUniverseE, varerr::Row<E<1>>, varerr::Row<E<0>, const E<1>, E<2>>>);

}

TEST_CASE("varerr_algebra_qualified_operations_unit", "[varerr][algebra]") {

    STATIC_REQUIRE(std::same_as<
        varerr::row_union_normalized_t<ConstUniverseE,
            varerr::Row<E<0>, const E<0>, E<1>, E<2>, const E<2>>,
            varerr::Row<E<0>, const E<0>, const E<1>, E<2>, const E<2>>
        >,
        varerr::Row<E<0>, const E<0>, E<1>, const E<1>, E<2>, const E<2>>
    >);

    STATIC_REQUIRE(std::same_as<
        varerr::row_intersection_normalized_t<ConstUniverseE,
            varerr::Row<E<0>, const E<0>, E<1>, E<2>, const E<2>>,
            varerr::Row<E<0>, const E<0>, const E<1>, E<2>, const E<2>>
        >,
        varerr::Row<E<0>, const E<0>, E<2>, const E<2>>
    >);

    STATIC_REQUIRE(std::same_as<
        varerr::row_difference_normalized_t<ConstUniverseE,
            varerr::Row<E<0>, const E<0>, E<1>, const E<1>, E<2>, const E<2>>,
            varerr::Row<E<0>, E<1>, const E<1>, const E<2>>
        >,
        varerr::Row<const E<0>, E<2>>
    >);

}

TEST_CASE("varerr_algebra_constraints_unit", "[varerr][algebra]") {

    STATIC_REQUIRE_FALSE(IsRowUnionNormalizedWellFormed<UniverseE, varerr::Row<E<1>, E<0>>>);
    STATIC_REQUIRE_FALSE(IsRowUnionNormalizedWellFormed<UniverseE, varerr::Row<E<0>, int>>);
    STATIC_REQUIRE_FALSE(IsRowUnionNormalizedWellFormed<UniverseE, L<3>, varerr::Row<E<1>, E<0>>>);
    STATIC_REQUIRE_FALSE(IsRowUnionNormalizedWellFormed<UniverseE, E<0>, L<4>>);
    STATIC_REQUIRE_FALSE(IsRowLookupNormalizedWellFormed<UniverseE, E<0>, varerr::Row<E<1>, E<0>>>);
    STATIC_REQUIRE_FALSE(IsRowLookupNormalizedWellFormed<UniverseH, E<0>, L<2>>);

}

TEST_CASE("varerr_algebra_distinct_rank", "[varerr][algebra]") {

    // No specified winner when distinct types share a rank.

    using U0 = varerr::row_union_normalized_t<ConstUniverseE, varerr::Row<E<0>&>, varerr::Row<volatile E<0>>>;
    STATIC_REQUIRE(varerr::row_size_v<U0> == 1);
    STATIC_REQUIRE(varerr::IsNormalizedRow<ConstUniverseE, U0>);

}
