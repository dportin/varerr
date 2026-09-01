#include <catch2/catch_test_macros.hpp>

#include "include/utilities.hpp"
#include "include/universe.hpp"

#include <varerr/status.hpp>

#include <array>
#include <bit>
#include <cstddef>
#include <type_traits>
#include <utility>

// Tests for row algebra implementation.

using namespace varerr::tests;
using namespace varerr::tests::universe;

namespace {

constexpr std::size_t kErrRowMaxElems = 4;
constexpr std::size_t kErrRowMaskBound = std::size_t {1} << kErrRowMaxElems;

// Prepend an element to a Row.

template <typename E, typename Row>
struct row_cons;

template <typename E, typename... Es>
struct row_cons<E, varerr::Row<Es...>> : std::type_identity<varerr::Row<E, Es...>> {};

template <typename E, typename Row>
using row_cons_t = row_cons<E, Row>::type;

template <typename E, typename Redex>
using row_cons_adapter = row_cons<E, typename Redex::type>;

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

// Naive computation of the nth set bit in a bitmask.

[[nodiscard]] constexpr std::size_t nth_set_bit(std::size_t mask, std::size_t n) noexcept {

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

// Convert a Lehmer code into a row of E<I> elements.

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
        // the decoded element (i.e., the number of inversions in the reduced u-
        // niverse). The decoded element is thus the (coefficient + 1)th set bit
        // in the current mask.

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

// Build compile-time comparison table between row and bitset operations.

template <std::size_t Rows, std::size_t Cols, std::size_t Row, typename Pred, std::size_t... Col>
consteval void build_compare_table_cols(std::array<bool, Rows * Cols>& table, Pred pred, std::index_sequence<Col...>) {
    ((table[Row * Cols + Col] = pred.template operator()<Rows, Cols, Row, Col>()), ...);
}

template <std::size_t Rows, std::size_t Cols, typename Pred, std::size_t... Row>
consteval void build_compare_table_rows(std::array<bool, Rows * Cols>& table, Pred pred, std::index_sequence<Row...>) {
    (build_compare_table_cols<Rows, Cols, Row>(table, pred, std::make_index_sequence<Cols> {}), ...);
}

template <std::size_t Rows, std::size_t Cols, typename Pred>
consteval auto build_compare_table(Pred pred) {
    std::array<bool, Rows * Cols> table {};
    build_compare_table_rows<Rows, Cols>(table, pred, std::make_index_sequence<Rows> {});
    return table;
}

template <std::size_t Rows, std::size_t Cols, typename Pred>
void row_algebra_compare_test(Pred pred) {

    constexpr auto buffer = build_compare_table<Rows, Cols>(pred);

    for (std::size_t row = 0; row < Rows; ++row) {
        for (std::size_t col = 0; col < Cols; ++col) {
            CAPTURE(row, col);
            REQUIRE(buffer[row * Cols + col]);
        }
    }

}

} // namespace

// Missing uniqueness/de-duplication coverage for row_normalize_t.

TEST_CASE("row_algebra_normalize_permutation", "[row_algebra]") {
    iterate_index_sequence<kErrRowMaskBound>([](const auto mask) -> void {
        constexpr std::size_t Mask = decltype(mask)::value;
        constexpr std::size_t MaskElems = std::popcount(Mask);
        constexpr std::size_t MaskPerms = factorial(MaskElems);
        row_algebra_compare_test<1, MaskPerms>(
            []<std::size_t, std::size_t, std::size_t, std::size_t Code>() consteval -> bool {
                return std::same_as<varerr::row_normalize_t<Universe, lehmer_code_to_row_t<Code, Mask>>, mask_to_row_t<Mask>>;
            }
        );
    });
}

TEST_CASE("row_algebra_subset_normalized", "[row_algebra]") {
    row_algebra_compare_test<kErrRowMaskBound, kErrRowMaskBound>(
        []<std::size_t, std::size_t, std::size_t N, std::size_t M>() consteval -> bool {
            return varerr::row_subset_normalized_v<Universe, mask_to_row_t<N>, mask_to_row_t<M>> == ((N & ~M) == 0);
        }
    );
}

TEST_CASE("row_algebra_equiv_normalized", "[row_algebra]") {
    row_algebra_compare_test<kErrRowMaskBound, kErrRowMaskBound>(
        []<std::size_t, std::size_t, std::size_t N, std::size_t M>() consteval -> bool {
            return varerr::row_equiv_normalized_v<Universe, mask_to_row_t<N>, mask_to_row_t<M>> == (N == M);
        }
    );
}

TEST_CASE("row_algebra_lookup_normalized", "[row_algebra]") {
    row_algebra_compare_test<kErrRowMaxElems, kErrRowMaskBound>(
        []<std::size_t, std::size_t, std::size_t N, std::size_t M>() consteval -> bool {
            constexpr auto result = varerr::row_lookup_normalized_v<Universe, E<N>, mask_to_row_t<M>>;
            constexpr auto present = static_cast<bool>((M >> N) & std::size_t {1});
            constexpr auto position = static_cast<std::size_t>(std::popcount(M & ((std::size_t {1} << N) - 1)));
            return present ? result.has_value() && result.value() == position : !result.has_value();
        });
}

TEST_CASE("row_algebra_union_normalized", "[row_algebra]") {
    row_algebra_compare_test<kErrRowMaskBound, kErrRowMaskBound>(
        []<std::size_t, std::size_t, std::size_t N, std::size_t M>() consteval -> bool {
            return std::same_as<varerr::row_union_normalized_t<Universe, mask_to_row_t<N>, mask_to_row_t<M>>, mask_to_row_t<N | M>>;
        }
    );
}

TEST_CASE("row_algebra_intersection_normalized", "[row_algebra]") {
    row_algebra_compare_test<kErrRowMaskBound, kErrRowMaskBound>(
        []<std::size_t, std::size_t, std::size_t N, std::size_t M>() consteval -> bool {
            return std::same_as<varerr::row_intersection_normalized_t<Universe, mask_to_row_t<N>, mask_to_row_t<M>>, mask_to_row_t<N & M>>;
        }
    );
}

TEST_CASE("row_algebra_difference_normalized", "[row_algebra]") {
    row_algebra_compare_test<kErrRowMaskBound, kErrRowMaskBound>(
        []<std::size_t, std::size_t, std::size_t N, std::size_t M>() consteval -> bool {
            return std::same_as<varerr::row_difference_normalized_t<Universe, mask_to_row_t<N>, mask_to_row_t<M>>, mask_to_row_t<N & ~M>>;
        }
    );
}
