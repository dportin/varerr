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

    template <typename E>
    concept IsStorable =
        std::is_object_v<E> &&
        std::same_as<E, std::remove_cv_t<E>>;

} // namespace detail

// The alternatives must be trivially copyable. Note that trivial copyability
// implies trivial destructibility since all trivially copyable types have a
// non-deleted trivial destructor. The requirement is repeated for clarity.

template <typename E>
concept IsTriviallyStorable =
    detail::IsStorable<E> &&
    std::is_trivially_copyable_v<E> &&
    std::is_trivially_destructible_v<E>;

namespace detail {

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

// Determine the position of an alternative in a row.

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

    // Lift row_lookup_normal_impl() to rows.

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

namespace detail {

    template <typename F, typename Seq>
    inline constexpr bool is_nothrow_invocable_over_sequence_v = false;

    template <typename F, std::size_t... Is>
    inline constexpr bool is_nothrow_invocable_over_sequence_v<F, std::index_sequence<Is...>> =
        (std::is_nothrow_invocable_v<F, std::integral_constant<std::size_t, Is>> && ...);

    template <typename F, std::size_t N>
    inline constexpr bool is_nothrow_invocable_over_index_sequence_v =
        is_nothrow_invocable_over_sequence_v<F, std::make_index_sequence<N>>;

    // Simple implementation of compile-time switch over indices.

    template <std::size_t N, typename F>
    requires (N > 0)
    [[nodiscard]] constexpr decltype(auto) dispatch_linear_dense(std::size_t n, F&& f)
    noexcept(is_nothrow_invocable_over_index_sequence_v<F, N>) /* conservative */ {

        assert(n < N); // precondition

        return [&]<std::size_t I>(this auto&& self) -> decltype(auto) {
            if constexpr (I + 1 == N) {
                return std::forward<F>(f)(std::integral_constant<std::size_t, I>{});
            } else {
                if (n == I) {
                    return std::forward<F>(f)(std::integral_constant<std::size_t, I>{});
                } else {
                    return self.template operator()<I + 1>();
                }
            }
        }.template operator()<0>();
    }

    template <std::size_t N, typename F>
    requires (N > 0)
    [[nodiscard]] constexpr decltype(auto) dispatch(std::size_t n, F&& f)
    noexcept(is_nothrow_invocable_over_index_sequence_v<F, N>) /* conservative */ {
        return dispatch_linear_dense<N>(n, std::forward<F>(f));
    }

    template <typename F, typename... Es>
    inline constexpr bool is_nothrow_visitable_v =
        (std::is_nothrow_invocable_v<F, const Es&> && ...);

    template <typename R, IsTriviallyStorable... Es>
    requires IsNormalized<R, Es...>
    struct StatusImpl final {

        static_assert(sizeof...(Es) > 0);

        using TagType = std::size_t;
        using StorageType = Storage<Es...>;

        // Construct a StatusImpl from an alternative.

        template <typename E, typename... Args>
        requires row_elem_v<R, E, Es...> &&
                 std::constructible_from<E, Args...>
        constexpr explicit StatusImpl(std::in_place_type_t<E>, Args&&... args)
        noexcept(std::is_nothrow_constructible_v<E, Args...>) :
            active_(row_index_v<R, E, Es...>),
            alternatives_(std::in_place_index<row_index_v<R, E, Es...>>, std::forward<Args>(args)...) {}

        // Implicit widening constructor.

        template <IsTriviallyStorable... Fs>
        requires IsNormalized<R, Fs...> &&
                 row_subset_v<R, Row<Fs...>, Row<Es...>> /* lifted */
        constexpr StatusImpl(const StatusImpl<R, Fs...>& other) noexcept :
            active_(0), alternatives_() {
                other.visit([this]<typename E>(const E& e) -> void {
                    constexpr std::size_t I = row_index_v<R, E, Es...>;
                    this->active_ = I;
                    storage_emplace<I>(this->alternatives_, e);
                });
            }

        // Dispatch visitor to active member by index.

        template <typename F>
        constexpr decltype(auto) visit(F&& f) const
        noexcept(is_nothrow_visitable_v<F, Es...>) /* conservative */ {
            return dispatch<sizeof...(Es)>(
                this->active_,
                [&]<std::size_t I>(std::integral_constant<std::size_t, I>) -> decltype(auto) {
                    return std::forward<F>(f)(storage_get<I>(this->alternatives_));
                }
            );
        }

        // Determine which alternative is active.

        template <typename E>
        requires row_elem_v<R, E, Es...>
        [[nodiscard]] constexpr bool holds() const noexcept {
            if constexpr (row_elem_v<R, E, Es...>) {
                return this->active_ == row_index_v<R, E, Es...>;
            } else {
                return false;
            }
        }

        private:

        TagType active_;
        StorageType alternatives_;

    };

    template <typename R>
    struct StatusImpl<R> final {

        // StatusImpl<R> is uninhabited: no value of this type should exist be-
        // cause the empty row has no alternatives. The copy and move construc-
        // tors are defaulted only because std::expected expects copy construc-
        // tibility.

        StatusImpl() = delete;

        StatusImpl(const StatusImpl& other) noexcept = default;
        StatusImpl& operator=(const StatusImpl&) noexcept = default;

        StatusImpl(StatusImpl&& other) noexcept = default;
        StatusImpl& operator=(StatusImpl&& other) noexcept = default;

        ~StatusImpl() noexcept = default;

    };

    // Normalize types by rank.

    template <typename R, typename... Es>
    requires IsRanked<R, Es...>
    consteval auto row_normalize_indices() noexcept {

        constexpr std::size_t N = sizeof...(Es);
        constexpr std::array<std::size_t, N> ranks { rank_v<R, Es>... };

        std::array<std::size_t, N> indices {};
        std::ranges::iota(indices, static_cast<std::size_t>(0));
        std::ranges::sort(indices, {}, [&](std::size_t i) { return ranks[i]; });

        // If !std::same_as<E, F> but rank_v<R, E> == rank_v<R, F> the user has
        // likely made an error defining their rank function, which is required
        // to be injective over their universe of error types. It might be use-
        // ful to check whether ranks preserve structural equality here: since
        // this is a consteval function throwing would not contaminate any run-
        // time code and would catch the most likely source of errors.

        std::size_t count = 0;
        for (std::size_t i = 0; i < N; ++i) {
            if (i == 0 || ranks[indices[i]] != ranks[indices[count - 1]]) {
                indices[count++] = indices[i];
            }
        }

        return std::pair { count, indices };

    }

    // Index into a template parameter pack. Falls back to std::tuple_element_t
    // if the compiler does not have a pack indexing builtin (MSVC, AppleClang).

    #if defined(__has_builtin) && __has_builtin(__type_pack_element)
    template <std::size_t I, typename... Ts>
    using pack_subscript_t = __type_pack_element<I, Ts...>;
    #else
    template <std::size_t I, typename... Ts>
    using pack_subscript_t = std::tuple_element_t<I, std::tuple<Ts...>>;
    #endif

    template <typename R, typename... Es>
    requires IsRanked<R, Es...>
    inline constexpr auto row_normalize_indices_v = row_normalize_indices<R, Es...>();

    template <typename R, typename... Es>
    requires IsRanked<R, Es...>
    struct row_normalize_impl {

        static constexpr auto result = row_normalize_indices_v<R, Es...>;
        static constexpr auto count = result.first;
        static constexpr auto indices = result.second;

        using type = decltype(
            []<std::size_t... Is>(std::index_sequence<Is...>) {
                return []<std::size_t... Js>() -> Row<pack_subscript_t<Js, Es...>...> {
                    return {};
                }.template operator()<indices[Is]...>();
            }(std::make_index_sequence<count> {})
        );

    };

    template <typename R, typename... Es>
    requires IsRanked<R, Es...>
    using row_normalize_t = row_normalize_impl<R, Es...>::type;

    // Unpack a Row into a StatusImpl.

    template <typename R, typename Row>
    struct row_status_adapter;

    template <typename R, typename... Es>
    struct row_status_adapter<R, Row<Es...>> : std::type_identity<StatusImpl<R, Es...>> {};

    template <typename R, typename Row>
    using row_status_adapter_t = typename row_status_adapter<R, Row>::type;

} // namespace detail

template <typename R, IsTriviallyStorable... Es>
requires IsRanked<R, Es...>
using Status = typename detail::row_status_adapter<R, detail::row_normalize_t<R, Es...>>::type;

} // namespace varerr

#endif // VARERR_VARSET_HPP
