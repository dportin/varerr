#ifndef VARERR_VARSET_HPP
#define VARERR_VARSET_HPP

#include <cstddef>

#include <algorithm>
#include <array>
#include <concepts>
#include <functional>
#include <memory>
#include <numeric>
#include <optional>
#include <ranges>
#include <type_traits>
#include <utility>

namespace varerr {

namespace detail {

    // The alternatives must be trivially copyable. Note that trivial copyabil-
    // ity implies trivial destructibility since all trivially copyable types
    // must have a non-deleted trivial destructor. The requirement is however
    // repeated here for clarity.

    template <typename E>
    concept IsStorable =
        std::is_object_v<E> && std::same_as<E, std::remove_cv_t<E>>;

    template <typename E>
    concept IsTriviallyStorable =
        IsStorable<E> && std::is_trivially_copyable_v<E> && std::is_trivially_destructible_v<E>;

    template <IsTriviallyStorable... Es>
    union Storage {

        constexpr Storage() noexcept {}

    };

    template <IsTriviallyStorable E, IsTriviallyStorable... Es>
    union Storage<E, Es...> {

        E head_;
        Storage<Es...> tail_;

        // P1331R2 removes the requirements that every non-variant non-static
        // data member and base class sub-object must be initialized in const-
        // expr contexts (provided the uninitialized values are not accessed).

        constexpr Storage() noexcept {}

        template <typename... Args>
        requires std::constructible_from<E, Args...>
        constexpr explicit Storage(std::in_place_index_t<0>, Args&&... args)
        noexcept(std::is_nothrow_constructible_v<E, Args...>) :
            head_(std::forward<Args>(args)...) {}

        template <std::size_t N, typename... Args>
        requires (N > 0 && N <= sizeof...(Es)) &&
                 std::constructible_from<Storage<Es...>, std::in_place_index_t<N - 1>, Args...>
        constexpr explicit Storage(std::in_place_index_t<N>, Args&&... args)
        noexcept(std::is_nothrow_constructible_v<Storage<Es...>, std::in_place_index_t<N - 1>, Args...>) :
            tail_(std::in_place_index<N - 1>, std::forward<Args>(args)...) {}

    };

    template <typename S>
    struct is_storage : std::false_type {};

    template <IsTriviallyStorable... Es>
    struct is_storage<Storage<Es...>> : std::true_type {};

    template <typename S>
    inline constexpr bool is_storage_v = is_storage<S>::value;

    template <typename S>
    concept IsStorage = is_storage_v<std::remove_cvref_t<S>>;

    // Determine the number of alternatives.

    template <typename S>
    struct storage_size;

    template <IsTriviallyStorable... Es>
    struct storage_size<Storage<Es...>> : std::integral_constant<std::size_t, sizeof...(Es)> {};

    template <IsStorage S>
    inline constexpr std::size_t storage_size_v = storage_size<std::remove_cvref_t<S>>::value;

    // Determine the type of the alternative at index N.

    template <std::size_t N, typename S>
    struct storage_alternative;

    template <IsTriviallyStorable E, IsTriviallyStorable... Es>
    struct storage_alternative<0, Storage<E, Es...>> : std::type_identity<E> {};

    template <std::size_t N, IsTriviallyStorable E, IsTriviallyStorable... Es>
    struct storage_alternative<N, Storage<E, Es...>> : storage_alternative<N - 1, Storage<Es...>> {};

    template <std::size_t N, IsStorage S>
    requires (N < storage_size_v<S>)
    using storage_alternative_t = typename storage_alternative<N, std::remove_cvref_t<S>>::type;

    // Return a reference to the alternative at index N. Note that reading the
    // reference is undefined if it does not refer to the active alternative.

    template <std::size_t N, typename S>
    requires (N < storage_size_v<S>)
    [[nodiscard]] constexpr auto& storage_get(S& storage) noexcept {
        if constexpr (N == 0) {
            return storage.head_;
        } else {
            return storage_get<N - 1>(storage.tail_);
        }
    }

    // Activate each union on the path to the alternative at index N (leaving
    // the selected alternative as raw storage for the caller to construct in
    // place). The previously defined active alternative is discarded without
    // destruction (which is well-defined since every alternative is trivially
    // destructible by construction).

    template <std::size_t N, typename S>
    requires (!std::is_const_v<S>) && (N < storage_size_v<S>)
    [[nodiscard]] constexpr auto* storage_emplace_impl(S& storage) noexcept {
        if constexpr (N == 0) {
            return std::addressof(storage.head_);
        } else {
            std::construct_at(std::addressof(storage.tail_));
            return storage_emplace_impl<N - 1>(storage.tail_);
        }
    }

    // Activate (emplace-construct) the alternative at index N. Note that the
    // pointer returned by std::construct_at is never null.

    template <std::size_t N, typename S, typename... Args>
    requires (!std::is_const_v<S>) && (N < storage_size_v<S>) &&
             std::constructible_from<storage_alternative_t<N, S>, Args...>
    constexpr auto* storage_emplace(S& storage, Args&&... args)
    noexcept(std::is_nothrow_constructible_v<storage_alternative_t<N, S>, Args...>) {
        return std::construct_at(storage_emplace_impl<N>(storage), std::forward<Args>(args)...);
    }

}

// The carrier of the type-level algebra.

template <typename... Es>
struct Row {};

// Determine whether a type is a row.

template <typename T>
inline constexpr bool is_row_v = false;

template <typename... Es>
inline constexpr bool is_row_v<Row<Es...>> = true;

template <typename T>
concept IsRow = is_row_v<T>;

// Determine whether a template parameter pack is ranked.

template <typename R, typename... Es>
concept IsRanked = (requires {
    typename std::integral_constant<std::size_t, R::template rank<Es>>;
} && ...);

template <typename R, typename E>
requires IsRanked<R, E>
inline constexpr std::size_t rank_v = R::template rank<E>;

// Determine whether a type is a ranked row.

template <typename R, typename T>
inline constexpr bool is_ranked_row_v = false;

template <typename R, typename... Es>
inline constexpr bool is_ranked_row_v<R, Row<Es...>> = IsRanked<R, Es...>;

template <typename R, typename T>
concept IsRankedRow = IsRow<T> && is_ranked_row_v<R, T>;

// Determine whether a template parameter pack is normalized.

namespace detail {

    template <typename R, typename... Es>
    requires IsRanked<R, Es...>
    [[nodiscard]] consteval bool is_normalized() noexcept {

        constexpr std::size_t N = sizeof...(Es);
        constexpr std::array<std::size_t, N> ranks { rank_v<R, Es>... };

        return std::ranges::adjacent_find(ranks, std::ranges::greater_equal{}) == ranks.end();

    }

} // namespace detail

template <typename R, typename... Es>
concept IsNormalized = IsRanked<R, Es...> && detail::is_normalized<R, Es...>();

// Determine whether a type is a normalized row.

template <typename R, typename T>
inline constexpr bool is_normalized_row_v = false;

template <typename R, typename... Es>
inline constexpr bool is_normalized_row_v<R, Row<Es...>> = IsNormalized<R, Es...>;

template <typename R, typename T>
concept IsNormalizedRow = IsRankedRow<R, T> && is_normalized_row_v<R, T>;

// Determine the position of an alternative in a ranked or normalized row.

namespace detail {

    template <typename R, typename E, typename... Es>
    requires IsRanked<R, E> && IsRanked<R, Es...>
    [[nodiscard]] consteval std::optional<std::size_t> row_lookup_ranked_impl() noexcept {

        constexpr std::size_t N = sizeof...(Es);
        constexpr std::array<std::size_t, N> ranks { rank_v<R, Es>... };
        const auto it = std::ranges::find(ranks, rank_v<R, E>);

        if (it != ranks.end()) {
            return static_cast<std::size_t>(it - ranks.begin()); // todo - distance
        }

        return std::nullopt;

    }

    template <typename R, typename E, typename... Es>
    requires IsRanked<R, E> && IsNormalized<R, Es...>
    [[nodiscard]] consteval std::optional<std::size_t> row_lookup_normal_impl() noexcept {

        constexpr std::size_t N = sizeof...(Es);
        constexpr std::array<std::size_t, N> ranks { rank_v<R, Es>... };
        const auto it = std::ranges::lower_bound(ranks, rank_v<R, E>);

        if (it != ranks.end() && *it == rank_v<R, E>) {
            return static_cast<std::size_t>(std::distance(ranks.begin(), it));
        }

        return std::nullopt;

    }

    template <typename R, typename E, typename... Es>
    requires IsRanked<R, E> && IsNormalized<R, Es...>
    inline constexpr std::optional<std::size_t> row_lookup_normal_v = row_lookup_normal_impl<R, E, Es...>();

    template <typename R, typename E, typename... Es>
    requires IsRanked<R, E> && IsNormalized<R, Es...>
    inline constexpr bool row_elem_normal_v = row_lookup_normal_v<R, E, Es...>.has_value();

    template <typename R, typename E, typename... Es>
    requires IsRanked<R, E> && IsNormalized<R, Es...> && row_elem_normal_v<R, E, Es...>
    inline constexpr std::size_t row_index_normal_v = row_lookup_normal_v<R, E, Es...>.value();

    // Adapt row_lookup_normal_impl<R, E, Es...>() to Row<Es...>.

    template <typename R, typename E, typename T>
    struct row_lookup_normal_impl_adapter;

    template <typename R, typename E, typename... Es>
    requires IsRanked<R, E> && IsNormalized<R, Es...>
    struct row_lookup_normal_impl_adapter<R, E, Row<Es...>> {
        static constexpr std::optional<std::size_t> value = row_lookup_normal_impl<R, E, Es...>();
    };

} // namespace detail

template <typename R, typename E, typename T>
requires IsRanked<R, E> && IsNormalizedRow<R, T>
inline constexpr std::optional<std::size_t> row_lookup_v = detail::row_lookup_normal_impl_adapter<R, E, T>::value;

template <typename R, typename E, typename T>
requires IsRanked<R, E> && IsNormalizedRow<R, T>
inline constexpr bool row_elem_v = row_lookup_v<R, E, T>.has_value();

template <typename R, typename E, typename T>
requires IsRanked<R, E> && IsNormalizedRow<R, T> && row_elem_v<R, E, T>
inline constexpr std::size_t row_index_v = row_lookup_v<R, E, T>.value();

// Determine whether two normalized rows are equivalent.

namespace detail {

    template <typename R, typename... Es, typename... Fs>
    requires IsNormalized<R, Es...> && IsNormalized<R, Fs...>
    [[nodiscard]] consteval bool row_subset_normal_impl(Row<Es...>, Row<Fs...>) noexcept {

        constexpr std::size_t sizeE = sizeof...(Es);
        constexpr std::array<std::size_t, sizeE> arrayE { rank_v<R, Es>... };

        constexpr std::size_t sizeF = sizeof...(Fs);
        constexpr std::array<std::size_t, sizeF> arrayF { rank_v<R, Fs>... };

        // std::ranges::includes uses std::ranges::less with the std::identity
        // projection.

        return std::ranges::includes(arrayF, arrayE);

    }

    template <typename R, typename... Es, typename... Fs>
    requires IsNormalized<R, Es...> && IsNormalized<R, Fs...>
    [[nodiscard]] consteval bool row_equiv_normal_impl(Row<Es...>, Row<Fs...>) noexcept {

        constexpr std::size_t sizeE = sizeof...(Es);
        constexpr std::array<std::size_t, sizeE> arrayE { rank_v<R, Es>... };

        constexpr std::size_t sizeF = sizeof...(Fs);
        constexpr std::array<std::size_t, sizeF> arrayF { rank_v<R, Fs>... };

        // std::ranges::equal uses std::ranges::equal_to with the std::identity
        // projection.

        return std::ranges::equal(arrayE, arrayF);

    }

} // namespace detail

template <typename R, typename U, typename V>
requires IsNormalizedRow<R, U> && IsNormalizedRow<R, V>
inline constexpr bool row_subset_v = detail::row_subset_normal_impl<R>(U {}, V {});

template <typename R, typename U, typename V>
requires IsNormalizedRow<R, U> && IsNormalizedRow<R, V>
inline constexpr bool row_equiv_v = detail::row_equiv_normal_impl<R>(U {}, V {});

// The main status type is parameterized by a row while its implementation is
// parameterized by a pack (for convenience, since lifting packs to rows is ea-
// sier than unpacking rows).

namespace detail {

    template <typename R, IsTriviallyStorable... Es>
    requires IsNormalized<R, Es...>
    struct StatusImpl final {

        using TagType = std::size_t;
        using StorageType = Storage<Es...>;

        /* TODO */

        private:

        TagType active_;
        StorageType alternatives_;

    };

} // namespace detail

} // namespace varerr

#endif // VARERR_VARSET_HPP
