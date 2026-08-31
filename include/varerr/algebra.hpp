#ifndef VARERR_ALGEBRA_HPP
#define VARERR_ALGEBRA_HPP

#include <array>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <numeric>
#include <optional>
#include <span>
#include <type_traits>
#include <utility>

#ifndef __has_builtin
#define __has_builtin(x) 0
#endif

// This file implements the error row operations used to define the combinators
// for the main result class. Error rows are ordered sets of error types. Error
// types are ordered by a rank function, which is required to be injective. The
// rank function must be a static member template of a universe class.

namespace varerr {

// Determine whether a type is ranked.

template <typename M, typename E>
concept IsRanked = requires {
    typename std::integral_constant<std::size_t, M::template rank<E>>;
};

// The rank of a type is defined relative to a universe.

template <typename M, typename E>
requires IsRanked<M, E>
inline constexpr std::size_t rank_v = M::template rank<E>;

// The carrier of the type-level algebra.

template <typename... Es>
struct Row {};

// Determine whether a type is a row.

template <typename U>
inline constexpr bool is_row_v = false;

template <typename... Es>
inline constexpr bool is_row_v<Row<Es...>> = true;

template <typename U>
concept IsRow = is_row_v<U>;

// Determine whether a parameter pack is ranked.

template <typename M, typename... Es>
inline constexpr bool is_ranked_pack_v = (IsRanked<M, Es> && ...);

template <typename M, typename... Es>
concept IsRankedPack = is_ranked_pack_v<M, Es...>;

// Determined whether a type is a ranked row.

template <typename M, typename U>
inline constexpr bool is_ranked_row_v = false;

template <typename M, typename... Es>
inline constexpr bool is_ranked_row_v<M, Row<Es...>> = is_ranked_pack_v<M, Es...>;

template <typename M, typename U>
concept IsRankedRow = IsRow<U> && is_ranked_row_v<M, U>;

// Compute the size of a row.

template <typename U>
struct row_size;

template <typename... Es>
struct row_size<Row<Es...>> : std::integral_constant<std::size_t, sizeof...(Es)> {};

template <IsRow U>
inline constexpr std::size_t row_size_v = row_size<U>::value;

namespace detail {

// Determine whether a parameter pack is normalized.

template <typename M, typename... Es>
requires IsRankedPack<M, Es...>
[[nodiscard]] consteval bool is_normalized_pack_impl() noexcept {

    constexpr std::array<std::size_t, sizeof...(Es)> ranks { rank_v<M, Es>... };
    return std::ranges::adjacent_find(ranks, std::ranges::greater_equal {}) == ranks.end();

}

} // namespace detail

template <typename M, typename... Es>
inline constexpr bool is_normalized_pack_v = detail::is_normalized_pack_impl<M, Es...>();

template <typename M, typename... Es>
concept IsNormalizedPack = IsRankedPack<M, Es...> && is_normalized_pack_v<M, Es...>;

// Determine whether a type is a normalized row.

template <typename M, typename T>
inline constexpr bool is_normalized_row_v = false;

template <typename M, typename... Es>
inline constexpr bool is_normalized_row_v<M, Row<Es...>> = is_normalized_pack_v<M, Es...>;

template <typename M, typename U>
concept IsNormalizedRow = IsRankedRow<M, U> && is_normalized_row_v<M, U>;

namespace detail {

// Compute an array of ranks from a parameter pack.

template <typename M, typename... Es>
requires IsRankedPack<M, Es...>
inline constexpr auto pack_ranks_v = std::array<std::size_t, sizeof...(Es)> { rank_v<M, Es>... };

template <typename M, typename U>
struct row_ranks;

template <typename M, typename... Es>
requires IsRankedPack<M, Es...>
struct row_ranks<M, Row<Es...>> {
    static constexpr auto value = pack_ranks_v<M, Es...>;
};

template <typename M, typename U>
requires IsRankedRow<M, U>
inline constexpr auto row_ranks_v = row_ranks<M, U>::value;

// Determine the position of an alternative in a parameter pack.

template <typename M, typename E, typename... Es>
requires IsRanked<M, E> &&
         IsNormalizedPack<M, Es...>
[[nodiscard]] consteval std::optional<std::size_t> pack_lookup_normalized_impl() {

    constexpr auto ranks = pack_ranks_v<M, Es...>;
    const auto it = std::ranges::lower_bound(ranks, rank_v<M, E>);

    if (it != ranks.end() && *it == rank_v<M, E>) {
        return static_cast<std::size_t>(std::distance(ranks.begin(), it));
    }

    return std::nullopt;

}

} // namespace detail

// The pack-level lookup operations are currently orphaned but publicly exposed
// for symmetry with the row-level lookup operations.

template <typename M, typename E, typename... Es>
requires IsRanked<M, E> &&
         IsNormalizedPack<M, Es...>
inline constexpr std::optional<std::size_t> pack_lookup_normalized_v = detail::pack_lookup_normalized_impl<M, E, Es...>();

template <typename M, typename E, typename... Es>
requires IsRanked<M, E> &&
         IsNormalizedPack<M, Es...>
inline constexpr bool pack_elem_normalized_v = pack_lookup_normalized_v<M, E, Es...>.has_value();

template <typename M, typename E, typename... Es>
requires IsRanked<M, E> &&
         IsNormalizedPack<M, Es...> &&
         pack_elem_normalized_v<M, E, Es...>
inline constexpr std::size_t pack_index_normalized_v = pack_lookup_normalized_v<M, E, Es...>.value();

// Lift lookup operations to rows.

namespace detail {

template <typename M, typename E, typename T>
struct row_lookup_normalized_impl_adapter;

template <typename M, typename E, typename... Es>
requires IsRanked<M, E> && IsNormalizedPack<M, Es...>
struct row_lookup_normalized_impl_adapter<M, E, Row<Es...>> {
    static constexpr std::optional<std::size_t> value = detail::pack_lookup_normalized_impl<M, E, Es...>();
};

} // namespace detail

// Determine the position of an alternative in a normalized row.

template <typename M, typename E, typename U>
requires IsRanked<M, E> &&
         IsNormalizedRow<M, U>
inline constexpr std::optional<std::size_t> row_lookup_normalized_v = detail::row_lookup_normalized_impl_adapter<M, E, U>::value;

template <typename M, typename E, typename U>
requires IsRanked<M, E> &&
         IsNormalizedRow<M, U>
inline constexpr bool row_elem_normalized_v = row_lookup_normalized_v<M, E, U>.has_value();

template <typename M, typename E, typename U>
requires IsRanked<M, E> &&
         IsNormalizedRow<M, U> &&
         row_elem_normalized_v<M, E, U>
inline constexpr std::size_t row_index_normalized_v = row_lookup_normalized_v<M, E, U>.value();

// Determine whether two normalized rows are equivalent.

namespace detail {

template <typename M, typename... Es, typename... Fs>
requires IsNormalizedPack<M, Es...> &&
         IsNormalizedPack<M, Fs...>
[[nodiscard]] consteval bool row_subset_normalized_impl(Row<Es...>, Row<Fs...>) {

    constexpr auto arrayE = pack_ranks_v<M, Es...>;
    constexpr auto arrayF = pack_ranks_v<M, Fs...>;

    // std::ranges::includes uses std::ranges::less with the std::identity pro-
    // jection.

    return std::ranges::includes(arrayF, arrayE);

}

template <typename M, typename... Es, typename... Fs>
requires IsNormalizedPack<M, Es...> &&
         IsNormalizedPack<M, Fs...>
[[nodiscard]] consteval bool row_equiv_normalized_impl(Row<Es...>, Row<Fs...>) {

    constexpr auto arrayE = pack_ranks_v<M, Es...>;
    constexpr auto arrayF = pack_ranks_v<M, Fs...>;

    // std::ranges::equal uses std::ranges::equal_to with the std::identity pro-
    // jection.

    return std::ranges::equal(arrayE, arrayF);

}

} // namespace detail

template <typename M, typename U, typename V>
requires IsNormalizedRow<M, U> &&
         IsNormalizedRow<M, V>
inline constexpr bool row_subset_normalized_v = detail::row_subset_normalized_impl<M>(U {}, V {});

template <typename M, typename U, typename V>
requires IsNormalizedRow<M, U> &&
         IsNormalizedRow<M, V>
inline constexpr bool row_equiv_normalized_v = detail::row_equiv_normalized_impl<M>(U {}, V {});

template <typename M, typename U, typename V>
requires IsNormalizedRow<M, U> &&
         IsNormalizedRow<M, V>
inline constexpr bool row_proper_subset_normalized_v = row_subset_normalized_v<M, U, V> && !row_equiv_normalized_v<M, U, V>;

namespace detail {

// Normalized parameter pack by rank.

template <typename M, typename... Es>
requires IsRankedPack<M, Es...>
consteval auto pack_normalize_indices() noexcept {

    constexpr std::size_t N = sizeof...(Es);
    constexpr std::array<std::size_t, N> ranks = pack_ranks_v<M, Es...>;

    std::array<std::size_t, N> indices {};
    std::ranges::iota(indices, static_cast<std::size_t>(0));
    std::ranges::sort(indices, {}, [&](std::size_t i) { return ranks[i]; });

    // If std::same_as<E, F> is false but rank_v<M, E> == rank_v<M, F> the user
    // has likely made an error defining their rank function, which must be in-
    // jective over the universe of error types. We should check whether ranks
    // preserve structural type equality here: since this function is consteval
    // throwing would not contaminate any runtime code and would catch the most
    // likely source of errors.

    std::size_t count = 0;
    for (std::size_t i = 0; i < N; ++i) {
        if (i == 0 || ranks[indices[i]] != ranks[indices[count - 1]]) {
            indices[count++] = indices[i];
        }
    }

    return std::pair { count, indices };

}

template <typename M, typename... Es>
requires IsRankedPack<M, Es...>
inline constexpr auto pack_normalize_indices_v = pack_normalize_indices<M, Es...>();

// Index into a parameter pack. Falls back to std::tuple_element_t if the compi-
// ler does not support pack indexing at the specified language revision or pro-
// vide a pack indexing builtin (MSVC, AppleClang).

#if defined(__cpp_pack_indexing) && __cpp_pack_indexing >= 202311L && __cplusplus > 202302L
template <std::size_t I, typename... Ts>
using pack_subscript_impl_t = Ts...[I];
#elif __has_builtin(__type_pack_element)
template <std::size_t I, typename... Ts>
using pack_subscript_impl_t = __type_pack_element<I, Ts...>;
#else
#include <tuple>
template <std::size_t I, typename... Ts>
using pack_subscript_impl_t = std::tuple_element_t<I, std::tuple<Ts...>>;
#endif

template <std::size_t I, typename... Ts>
struct pack_subscript : std::type_identity<pack_subscript_impl_t<I, Ts...>> {};

template <std::size_t I, typename... Ts>
using pack_subscript_t = pack_subscript<I, Ts...>::type;

// Index into a row.

template <std::size_t I, typename U>
struct row_subscript;

template <std::size_t I, typename... Es>
struct row_subscript<I, Row<Es...>> : std::type_identity<pack_subscript_t<I, Es...>> {};

template <std::size_t I, IsRow U>
using row_subscript_t = row_subscript<I, U>::type;

} // namespace detail

// Normalize a parameter pack.

template <typename M, typename... Es>
requires IsRankedPack<M, Es...>
struct pack_normalize {

    // pack_normalize<M, Es...>()::type is Row<Fs...> where Fs... is a subset of
    // Es... that is sorted and unique with respect to R::rank.

    static constexpr auto result = detail::pack_normalize_indices_v<M, Es...>;
    static constexpr auto count = result.first;
    static constexpr auto indices = result.second;

    // NOTE: Factored nested parameter pack out of lambda return type to satisfy
    // MSVC parser.

    using row = Row<Es...>;

    using type = decltype(
        []<std::size_t... Is>(std::index_sequence<Is...>) {
            return []<std::size_t... Js>() -> Row<detail::row_subscript_t<Js, row>...> {
                return {};
            }.template operator()<indices[Is]...>();
        }(std::make_index_sequence<count> {})
    );

};

template <typename M, typename... Es>
requires IsRankedPack<M, Es...>
using pack_normalize_t = pack_normalize<M, Es...>::type;

// Normalize a row.

template <typename M, typename U>
struct row_normalize;

template <typename M, typename... Es>
struct row_normalize<M, Row<Es...>> : pack_normalize<M, Es...> {};

template <typename M, typename U>
requires IsRankedRow<M, U>
using row_normalize_t = row_normalize<M, U>::type;

namespace detail {

// Compute the union, intersection and difference of two or more rows.

enum class MergeOp : std::uint8_t { Union, Intersection, Difference };

// Track the source of an index when merging parameter packs.

struct MergeStep {
    std::size_t index;
    std::size_t source;
};

// Bound the number of merge steps by the sum of the sizes of the individual pa-
// rameter packs.

template <std::size_t N>
struct MergePlan {
    std::size_t size;
    std::array<MergeStep, N> steps;
};

// Naive (linear) merge with union, intersection and difference operations.

template <MergeOp Op, typename M, typename... Rows>
requires (IsNormalizedRow<M, Rows> && ...)
[[nodiscard]] consteval auto merge_normalized_rows_linear_impl() {

    constexpr std::size_t num_rows = sizeof...(Rows);
    constexpr std::size_t num_ranks_max = std::size_t {0} + (row_size_v<Rows> + ...);

    // Ragged array of ranks.

    const std::array<std::span<const std::size_t>, num_rows> ranks_table {
        std::span<const std::size_t> { row_ranks_v<M, Rows> }...
    };

    MergePlan<num_ranks_max> merge_plan {};
    std::array<std::size_t, num_rows> ranks_table_col {};

    while (true) {

        std::size_t min_rank = 0;
        std::size_t min_rank_row = num_rows; /* sentinel */

        for (std::size_t row = 0; row < num_rows; ++row) {

            if (ranks_table_col[row] == ranks_table[row].size()) {
                continue;
            }

            const std::size_t rank = ranks_table[row][ranks_table_col[row]];

            if (min_rank_row == num_rows || rank < min_rank) {
                min_rank = rank;
                min_rank_row = row;
            }

        }

        // If the frontier is empty then there are no more elements to consider.

        if (min_rank_row == num_rows) {
            break;
        }

        // If the frontier is non-empty then min_rank is the minimum rank at row
        // min_rank_row and index min_rank_col.

        std::size_t min_rank_col = ranks_table_col[min_rank_row];

        // Count the number of occurrences of min_rank on the frontier. Merging
        // this with the de-duplication pass would incraese code complexity for
        // only minimal performance gains.

        std::size_t num_min_rank_frontier = 0;
        for (std::size_t row = 0; row < num_rows; ++row) {
            if (ranks_table_col[row] != ranks_table[row].size() &&
                ranks_table[row][ranks_table_col[row]] == min_rank) {
                ++num_min_rank_frontier;
            }
        }

        static_assert(Op == MergeOp::Union || Op == MergeOp::Intersection || Op == MergeOp::Difference,
            "merge_normalized_rows_linear_impl: unrecognized merge operation");

        bool add_merge_step = false;
        if constexpr (Op == MergeOp::Union) {
            add_merge_step = true;
        } else if constexpr (Op == MergeOp::Intersection) {
            add_merge_step = num_min_rank_frontier == num_rows;
        } else {
            add_merge_step = min_rank_row == 0 && num_min_rank_frontier == 1;
        }

        if (add_merge_step) {
            merge_plan.steps[merge_plan.size++] = MergeStep {
                .index = min_rank_col,
                .source = min_rank_row
            };
        }

        // Ensure the emitted rank indices are de-duplicated.

        for (std::size_t row = 0; row < num_rows; ++row) {
            while (ranks_table_col[row] != ranks_table[row].size() &&
                   ranks_table[row][ranks_table_col[row]] == min_rank) {
                ++ranks_table_col[row];
            }
        }

    }

    return merge_plan;

}

template <MergeOp Op, typename M, typename... Rows>
requires (IsNormalizedRow<M, Rows> && ...)
struct merge_normalized_rows_linear {

    static constexpr auto merge_plan = merge_normalized_rows_linear_impl<Op, M, Rows...>();
    static constexpr auto merge_size = merge_plan.size;
    static constexpr auto merge_steps = merge_plan.steps;

    // NOTE: Factored nested parameter pack out of lambda return type to satisfy
    // MSVC parser.

    using rows = Row<Rows...>;

    using type = decltype(
        []<std::size_t... Is>(std::index_sequence<Is...>) {
            return []<MergeStep... Ms>() ->Row<row_subscript_t<Ms.index, row_subscript_t<Ms.source, rows>>...> {
                return {};
            }.template operator()<merge_steps[Is]...>();
        }(std::make_index_sequence<merge_size> {})
    );

};

} // namespace detail

template <typename M, typename... Rows>
using row_union_normalized_t = detail::merge_normalized_rows_linear<detail::MergeOp::Union, M, Rows...>::type;

template <typename M, typename... Rows>
using row_intersection_normalized_t = detail::merge_normalized_rows_linear<detail::MergeOp::Intersection, M, Rows...>::type;

template <typename M, typename... Rows>
using row_difference_normalized_t = detail::merge_normalized_rows_linear<detail::MergeOp::Difference, M, Rows...>::type;

} // namespace varerr

#endif // VARERR_ALGEBRA_HPP
