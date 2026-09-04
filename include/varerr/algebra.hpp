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

// This file implements the error row operations used to define the combinators
// for the main result class. Error rows are ordered sets of error types. Error
// types are ordered by a rank function (which partitions the universe). The in-
// tended rank functions are insensitive to cvref-qualifiers (so that the equiv-
// alence classes are cvref-qualifications of a base type). However, that condi-
// tion is not enforced in this header. Rank functions must be defined as a sta-
// tic member template of a universe class that parameterizes the row operations
// in this header.

// Normalization and merge operations resolve rank collisions by position: row
// normalization retains the first occurrence in the pack; merge operations re-
// tain the leftmost occurrence in the rows. The membership, subset and equal-
// ity operations resolve element membership and equivalence by rank; thus two
// rows may compare equal without being structurally equal.

namespace varerr {

// Determine whether a type E is ranked relative to a universe M.

template <typename M, typename E>
concept IsRanked = requires {
    typename std::integral_constant<std::size_t, M::template rank<E>>;
};

// Determine the rank of a type E relative to a universe M.

template <typename M, typename E>
requires IsRanked<M, E>
inline constexpr std::size_t rank_v = M::template rank<E>;

// The (free) carrier of the row algebra.

template <typename... Es>
struct Row {};

// Determine whether a type is a Row.

namespace detail {

template <typename U>
struct is_row : std::false_type {};

template <typename... Es>
struct is_row<Row<Es...>> : std::true_type {};

} // namespace detail

template <typename U>
inline constexpr bool is_row_v = detail::is_row<std::remove_cvref_t<U>>::value;

template <typename U>
concept IsRow = is_row_v<U>;

// Determine whether a parameter pack is ranked.

template <typename M, typename... Es>
inline constexpr bool is_ranked_pack_v = (IsRanked<M, Es> && ...);

template <typename M, typename... Es>
concept IsRankedPack = is_ranked_pack_v<M, Es...>;

// Determine whether a type is a ranked Row.

namespace detail {

template <typename M, typename U>
struct is_ranked_row : std::false_type {};

template <typename M, typename... Es>
struct is_ranked_row<M, Row<Es...>> : std::bool_constant<is_ranked_pack_v<M, Es...>> {};

} // namespace detail

template <typename M, typename U>
inline constexpr bool is_ranked_row_v = detail::is_ranked_row<M, std::remove_cvref_t<U>>::value;

template <typename M, typename U>
concept IsRankedRow = IsRow<U> && is_ranked_row_v<M, U>;

// Compute the size of a Row.

namespace detail {

template <typename U>
struct row_size;

template <typename... Es>
struct row_size<Row<Es...>> : std::integral_constant<std::size_t, sizeof...(Es)> {};

} // namespace detail

template <IsRow U>
inline constexpr std::size_t row_size_v = detail::row_size<std::remove_cvref_t<U>>::value;

// Determine whether a parameter pack is normalized.

namespace detail {

template <typename M, typename... Es>
requires IsRankedPack<M, Es...>
[[nodiscard]] consteval bool is_normalized_pack_impl() {

    constexpr std::array<std::size_t, sizeof...(Es)> ranks { rank_v<M, Es>... };
    return std::ranges::adjacent_find(ranks, std::ranges::greater_equal {}) == ranks.end();

}

} // namespace detail

template <typename M, typename... Es>
requires IsRankedPack<M, Es...>
inline constexpr bool is_normalized_pack_v = detail::is_normalized_pack_impl<M, Es...>();

template <typename M, typename... Es>
concept IsNormalizedPack = IsRankedPack<M, Es...> && is_normalized_pack_v<M, Es...>;

// Determine whether a type is a normalized Row.

namespace detail {

template <typename M, typename U>
struct is_normalized_row : std::false_type {};

template <typename M, typename... Es>
struct is_normalized_row<M, Row<Es...>> : std::bool_constant<is_normalized_pack_v<M, Es...>> {};

} // namespace detail

template <typename M, typename U>
requires IsRankedRow<M, U>
inline constexpr bool is_normalized_row_v = detail::is_normalized_row<M, std::remove_cvref_t<U>>::value;

template <typename M, typename U>
concept IsNormalizedRow = IsRankedRow<M, U> && is_normalized_row_v<M, U>;

namespace detail {

// Transform a parameter pack into an array of ranks.

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

// Transform a Row into an array of ranks.

template <typename M, typename U>
requires IsRankedRow<M, U>
inline constexpr auto row_ranks_v = row_ranks<M, std::remove_cvref_t<U>>::value;

// Determine the position of an alternative in a parameter pack.

template <typename M, typename E, typename... Es>
requires IsRanked<M, E> &&
         IsNormalizedPack<M, Es...>
[[nodiscard]] consteval std::optional<std::size_t> row_lookup_normalized_impl(Row<Es...>) {

    constexpr auto ranks = pack_ranks_v<M, Es...>;
    const auto it = std::ranges::lower_bound(ranks, rank_v<M, E>);

    if (it != ranks.end() && *it == rank_v<M, E>) {
        return static_cast<std::size_t>(std::distance(ranks.begin(), it));
    }

    return std::nullopt;

}

} // namespace detail

// Determine the position of an alternative in a normalized Row.

template <typename M, typename E, typename U>
requires IsRanked<M, E> &&
         IsNormalizedRow<M, U>
inline constexpr std::optional<std::size_t> row_lookup_normalized_v =
    detail::row_lookup_normalized_impl<M, E>(std::remove_cvref_t<U> {});

template <typename M, typename E, typename U>
requires IsRanked<M, E> &&
         IsNormalizedRow<M, U>
inline constexpr bool row_elem_normalized_v = row_lookup_normalized_v<M, E, U>.has_value();

template <typename M, typename E, typename U>
requires IsRanked<M, E> &&
         IsNormalizedRow<M, U> &&
         row_elem_normalized_v<M, E, U>
inline constexpr std::size_t row_index_normalized_v = row_lookup_normalized_v<M, E, U>.value();

// Determine whether two normalized Rows are equivalent.

namespace detail {

template <typename M, typename... Es, typename... Fs>
requires IsNormalizedPack<M, Es...> &&
         IsNormalizedPack<M, Fs...>
[[nodiscard]] consteval bool row_subset_normalized_impl(Row<Es...>, Row<Fs...>) {

    constexpr auto arrayE = pack_ranks_v<M, Es...>;
    constexpr auto arrayF = pack_ranks_v<M, Fs...>;
    return std::ranges::includes(arrayF, arrayE);

}

template <typename M, typename... Es, typename... Fs>
requires IsNormalizedPack<M, Es...> &&
         IsNormalizedPack<M, Fs...>
[[nodiscard]] consteval bool row_equiv_normalized_impl(Row<Es...>, Row<Fs...>) {

    constexpr auto arrayE = pack_ranks_v<M, Es...>;
    constexpr auto arrayF = pack_ranks_v<M, Fs...>;
    return std::ranges::equal(arrayE, arrayF);

}

} // namespace detail

template <typename M, typename U, typename V>
requires IsNormalizedRow<M, U> &&
         IsNormalizedRow<M, V>
inline constexpr bool row_subset_normalized_v =
    detail::row_subset_normalized_impl<M>(std::remove_cvref_t<U> {}, std::remove_cvref_t<V> {});

template <typename M, typename U, typename V>
requires IsNormalizedRow<M, U> &&
         IsNormalizedRow<M, V>
inline constexpr bool row_equiv_normalized_v =
    detail::row_equiv_normalized_impl<M>(std::remove_cvref_t<U> {}, std::remove_cvref_t<V> {});

template <typename M, typename U, typename V>
requires IsNormalizedRow<M, U> &&
         IsNormalizedRow<M, V>
inline constexpr bool row_proper_subset_normalized_v =
    row_subset_normalized_v<M, U, V> && !row_equiv_normalized_v<M, U, V>;

namespace detail {

// Normalize a Row by rank.

template <typename M, typename... Es>
requires IsRankedPack<M, Es...>
[[nodiscard]] consteval auto pack_normalize_indices(Row<Es...>) {

    constexpr std::size_t N = sizeof...(Es);
    constexpr std::array<std::size_t, N> ranks = pack_ranks_v<M, Es...>;

    std::array<std::size_t, N> indices {};
    std::ranges::iota(indices, static_cast<std::size_t>(0));
    std::ranges::sort(indices, {}, [&](std::size_t i) { return ranks[i]; });

    // Select the minimum index to resolve rank collisions; std::stable_sort is
    // unfortunately not constexpr until C++26.

    std::size_t count = 0;
    for (std::size_t i = 0; i < N; ++i) {
        if (i == 0 || ranks[indices[i]] != ranks[indices[count - 1]]) {
            indices[count++] = indices[i];
        } else {
            indices[count - 1] = std::min(indices[count - 1], indices[i]);
        }
    }

    return std::pair { count, indices };

}

// Index into a parameter pack. Falls back to pack_subscript_recursive if the
// compiler does not support pack indexing at the specified language revision
// or provide a pack indexing builtin (MSVC, AppleClang).

template <std::size_t I, typename... Ts>
struct pack_subscript_recursive;

template <typename T, typename... Ts>
struct pack_subscript_recursive<0, T, Ts...> : std::type_identity<T> {};

template <std::size_t I, typename T, typename... Ts>
struct pack_subscript_recursive<I, T, Ts...> : pack_subscript_recursive<I - 1, Ts...> {};

#if defined(__has_builtin)
#define VARERR_HAS_BUILTIN(x) __has_builtin(x)
#else
#define VARERR_HAS_BUILTIN(x) 0
#endif

// MSVC reports the language revision in _MSVC_LANG unless /Zc:__cplusplus is
// set.

#if defined(_MSVC_LANG)
#define VARERR_LANGUAGE _MSVC_LANG
#else
#define VARERR_LANGUAGE __cplusplus
#endif

#if defined(__cpp_pack_indexing) && __cpp_pack_indexing >= 202311L && VARERR_LANGUAGE > 202302L
template <std::size_t I, typename... Ts>
using pack_subscript_impl_t = Ts...[I];
#elif VARERR_HAS_BUILTIN(__type_pack_element)
template <std::size_t I, typename... Ts>
using pack_subscript_impl_t = __type_pack_element<I, Ts...>;
#else
template <std::size_t I, typename... Ts>
using pack_subscript_impl_t = pack_subscript_recursive<I, Ts...>::type;
#endif

#undef VARERR_LANGUAGE
#undef VARERR_HAS_BUILTIN

template <std::size_t I, typename... Ts>
struct pack_subscript : std::type_identity<pack_subscript_impl_t<I, Ts...>> {};

template <std::size_t I, typename... Ts>
using pack_subscript_t = pack_subscript<I, Ts...>::type;

// Index into a Row.

template <std::size_t I, typename U>
struct row_subscript;

template <std::size_t I, typename... Es>
struct row_subscript<I, Row<Es...>> : std::type_identity<pack_subscript_t<I, Es...>> {};

template <std::size_t I, IsRow U>
using row_subscript_t = row_subscript<I, std::remove_cvref_t<U>>::type;

// Normalize a parameter pack.

template <typename M, typename... Es>
requires IsRankedPack<M, Es...>
struct pack_normalize {

    // pack_normalize<M, Es...>()::type is Row<Fs...> where Fs... is a subset of
    // Es... that is sorted and unique with respect to M::rank.

    static constexpr auto result = pack_normalize_indices<M>(Row<Es...> {});
    static constexpr auto count = result.first;
    static constexpr auto indices = result.second;

    // The nested parameter pack is factored out of the lambda return type to
    // satisfy the MSVC parser.

    using row = Row<Es...>;

    using type = decltype(
        []<std::size_t... Is>(std::index_sequence<Is...>) {
            return []<std::size_t... Js>() -> Row<row_subscript_t<Js, row>...> {
                return {};
            }.template operator()<indices[Is]...>();
        }(std::make_index_sequence<count> {})
    );

};

} // namespace detail

template <typename M, typename... Es>
requires IsRankedPack<M, Es...>
using pack_normalize_t = detail::pack_normalize<M, Es...>::type;

// Normalize a row.

namespace detail {

template <typename M, typename U>
struct row_normalize;

template <typename M, typename... Es>
struct row_normalize<M, Row<Es...>> : pack_normalize<M, Es...> {};

} // namespace detail

template <typename M, typename U>
requires IsRankedRow<M, U>
using row_normalize_t = detail::row_normalize<M, std::remove_cvref_t<U>>::type;

namespace detail {

// Compute the union, intersection and difference of two or more Rows.

enum class MergeOp : std::uint8_t {
    Union,
    Intersection,
    Difference
};

// Track the source of an index when merging parameter packs.

struct MergeStep {
    std::size_t index;
    std::size_t source;
};

// The number of merge steps is bounded by the sum of the sizes of the individ-
// ual parameter packs.

template <std::size_t N>
struct MergePlan {
    std::size_t size;
    std::array<MergeStep, N> steps;
};

// Naive (linear) merge with union, intersection and difference operations. Row
// is used here as a generic type-level list.

template <MergeOp Op, typename M, typename... Rows>
requires (IsNormalizedRow<M, Rows> && ...)
[[nodiscard]] consteval auto merge_normalized_rows_linear_impl(Row<Rows...>) {

    static_assert(Op == MergeOp::Union || Op == MergeOp::Intersection || Op == MergeOp::Difference,
        "merge_normalized_rows_linear_impl: unrecognized merge operation");

    constexpr std::size_t num_rows = sizeof...(Rows);
    constexpr std::size_t num_ranks_max = (std::size_t {0} + ... + row_size_v<Rows>);

    // Construct a ragged array of ranks from the Row parameters.

    const std::array<std::span<const std::size_t>, num_rows> ranks_table {
        std::span<const std::size_t> { row_ranks_v<M, Rows> }...
    };

    MergePlan<num_ranks_max> merge_plan {};
    std::array<std::size_t, num_rows> ranks_table_col {};

    while (true) {

        std::size_t min_rank = 0;
        std::size_t min_rank_row = num_rows; /* sentinel */

        // Sweep the frontier to find the element with the minimum rank.

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

        // If the frontier is empty there are no more elements to consider.

        if (min_rank_row == num_rows) {
            break;
        }

        // If the frontier is non-empty then min_rank is the minimum rank at row
        // min_rank_row and index min_rank_col.

        std::size_t min_rank_col = ranks_table_col[min_rank_row];

        // Count the number of occurrences of min_rank on the frontier. Merging
        // this with the de-duplication pass would increase code complexity for
        // minimal performance gains.

        std::size_t num_min_rank_frontier = 0;
        for (std::size_t row = 0; row < num_rows; ++row) {
            if (ranks_table_col[row] != ranks_table[row].size() &&
                ranks_table[row][ranks_table_col[row]] == min_rank) {
                ++num_min_rank_frontier;
            }
        }

        bool add_merge_step = false;
        if constexpr (Op == MergeOp::Union) {
            add_merge_step = true;
        } else if constexpr (Op == MergeOp::Intersection) {
            add_merge_step = num_min_rank_frontier == num_rows;
        } else if constexpr (Op == MergeOp::Difference) {
            add_merge_step = min_rank_row == 0 && num_min_rank_frontier == 1;
        } else {
            std::unreachable();
        }

        if (add_merge_step) {
            merge_plan.steps[merge_plan.size++] = MergeStep {
                .index = min_rank_col, .source = min_rank_row
            };
        }

        // Ensure the emitted indices are de-duplicated by rank.

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

    static constexpr auto merge_plan = merge_normalized_rows_linear_impl<Op, M>(Row<Rows...> {});
    static constexpr auto merge_size = merge_plan.size;
    static constexpr auto merge_steps = merge_plan.steps;

    // The nested parameter pack is factored out of the lambda return type to
    // satisfy the MSVC parser.

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
requires (sizeof...(Rows) > 0) &&
         (IsNormalizedRow<M, Rows> && ...)
using row_union_normalized_t = detail::merge_normalized_rows_linear<
    detail::MergeOp::Union, M, std::remove_cvref_t<Rows>...>::type;

template <typename M, typename... Rows>
requires (sizeof...(Rows) > 0) &&
         (IsNormalizedRow<M, Rows> && ...)
using row_intersection_normalized_t = detail::merge_normalized_rows_linear<
    detail::MergeOp::Intersection, M, std::remove_cvref_t<Rows>...>::type;

template <typename M, typename... Rows>
requires (sizeof...(Rows) > 0) &&
         (IsNormalizedRow<M, Rows> && ...)
using row_difference_normalized_t = detail::merge_normalized_rows_linear<
    detail::MergeOp::Difference, M, std::remove_cvref_t<Rows>...>::type;

} // namespace varerr

#endif // VARERR_ALGEBRA_HPP
