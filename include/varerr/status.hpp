#ifndef VARERR_STATUS_HPP
#define VARERR_STATUS_HPP

#include <cassert>
#include <cstddef>
#include <cstdint>

#include <algorithm>
#include <array>
#include <concepts>
#include <functional>
#include <memory>
#include <numeric>
#include <optional>
#include <span>
#include <type_traits>
#include <utility>

namespace varerr {

// The alternatives must be trivially copyable. Note that trivial copyability
// implies trivial destructibility since all trivially copyable types have a
// non-deleted trivial destructor. The requirement is repeated for clarity.

namespace detail {

template <typename E>
concept IsStorable =
    std::is_object_v<E> &&
    std::same_as<E, std::remove_cv_t<E>>;

} // namespace detail

template <typename E>
concept IsTriviallyStorable =
    detail::IsStorable<E> &&
    std::is_trivially_copyable_v<E> &&
    std::is_trivially_destructible_v<E>;

namespace detail {

    // The Storage type uses a recursive union to specialize the std::variant
    // storage to trivially copyable types.

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

} // namespace detail

// The carrier of the type-level algebra.

template <typename... Es>
struct Row {};

// Determine whether a type is a row.

template <typename T>
inline constexpr bool is_row_v = false;

template <typename... Es>
inline constexpr bool is_row_v<Row<Es...>> = true;

template <typename U>
concept IsRow = is_row_v<U>;

// Determine whether a parameter pack is ranked.

template <typename R, typename E>
concept IsRanked = requires {
    typename std::integral_constant<std::size_t, R::template rank<E>>;
};

namespace detail {

    template <typename R, typename... Es>
    inline constexpr bool is_ranked_pack_v = (IsRanked<R, Es> && ...);

    template <typename R, typename... Es>
    concept IsRankedPack = is_ranked_pack_v<R, Es...>;

} // namespace detail

// Determine whether a type is a ranked row.

template <typename R, typename T>
inline constexpr bool is_ranked_row_v = false;

template <typename R, typename... Es>
inline constexpr bool is_ranked_row_v<R, Row<Es...>> = detail::is_ranked_pack_v<R, Es...>;

template <typename R, typename U>
concept IsRankedRow = IsRow<U> && is_ranked_row_v<R, U>;

// The rank function must be injective.

template <typename R, typename E>
requires IsRanked<R, E>
inline constexpr std::size_t rank_v = R::template rank<E>;

// The size of a row

template <typename U>
struct row_size;

template <typename... Es>
struct row_size<Row<Es...>> : std::integral_constant<std::size_t, sizeof...(Es)> {};

template <IsRow U>
inline constexpr std::size_t row_size_v = row_size<U>::value;

// Determine whether a parameter pack is normalized.

namespace detail {

    template <typename R, typename... Es>
    requires IsRankedPack<R, Es...>
    [[nodiscard]] consteval bool is_normalized_pack() noexcept {

        constexpr std::size_t N = sizeof...(Es);
        constexpr std::array<std::size_t, N> ranks { rank_v<R, Es>... };

        return std::ranges::adjacent_find(ranks, std::ranges::greater_equal{}) == ranks.end();

    }

    template <typename R, typename... Es>
    inline constexpr bool is_normalized_pack_v = is_normalized_pack<R, Es...>();

    template <typename R, typename... Es>
    concept IsNormalizedPack = IsRankedPack<R, Es...> && is_normalized_pack_v<R, Es...>;

} // namespace detail

// Determine whether a type is a normalized row.

template <typename R, typename T>
inline constexpr bool is_normalized_row_v = false;

template <typename R, typename... Es>
inline constexpr bool is_normalized_row_v<R, Row<Es...>> = detail::is_normalized_pack_v<R, Es...>;

template <typename R, typename U>
concept IsNormalizedRow = IsRankedRow<R, U> && is_normalized_row_v<R, U>;

// Compute an array of ranks from a parameter pack.

namespace detail {

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

} // namespace detail

// Determine the position of an alternative in a parameter pack.

namespace detail {

    template <typename R, typename E, typename... Es>
    requires IsRanked<R, E> && IsRankedPack<R, Es...>
    [[nodiscard]] consteval std::optional<std::size_t> pack_lookup_ranked_impl() {

        constexpr std::size_t N = sizeof...(Es);
        constexpr std::array<std::size_t, N> ranks { rank_v<R, Es>... };
        const auto it = std::ranges::find(ranks, rank_v<R, E>);

        if (it != ranks.end()) {
            return static_cast<std::size_t>(it - ranks.begin()); // todo - distance
        }

        return std::nullopt;

    }

    template <typename R, typename E, typename... Es>
    requires IsRanked<R, E> && IsNormalizedPack<R, Es...>
    [[nodiscard]] consteval std::optional<std::size_t> pack_lookup_normalized_impl() {

        constexpr std::size_t N = sizeof...(Es);
        constexpr std::array<std::size_t, N> ranks { rank_v<R, Es>... };
        const auto it = std::ranges::lower_bound(ranks, rank_v<R, E>);

        if (it != ranks.end() && *it == rank_v<R, E>) {
            return static_cast<std::size_t>(std::distance(ranks.begin(), it));
        }

        return std::nullopt;

    }

    template <typename R, typename E, typename... Es>
    requires IsRanked<R, E> && IsNormalizedPack<R, Es...>
    inline constexpr std::optional<std::size_t> pack_lookup_normalized_v = pack_lookup_normalized_impl<R, E, Es...>();

    template <typename R, typename E, typename... Es>
    requires IsRanked<R, E> && IsNormalizedPack<R, Es...>
    inline constexpr bool pack_elem_normalized_v = pack_lookup_normalized_v<R, E, Es...>.has_value();

    template <typename R, typename E, typename... Es>
    requires IsRanked<R, E> && IsNormalizedPack<R, Es...> && pack_elem_normalized_v<R, E, Es...>
    inline constexpr std::size_t pack_index_normalized_v = pack_lookup_normalized_v<R, E, Es...>.value();

    // Lift pack_lookup_normalized_impl() to rows.

    template <typename R, typename E, typename T>
    struct row_lookup_normalized_impl_adapter;

    template <typename R, typename E, typename... Es>
    requires IsRanked<R, E> && IsNormalizedPack<R, Es...>
    struct row_lookup_normalized_impl_adapter<R, E, Row<Es...>> {
        static constexpr std::optional<std::size_t> value = pack_lookup_normalized_impl<R, E, Es...>();
    };

} // namespace detail

// Determine the position of an alternative in a normalized row.

template <typename R, typename E, typename U>
requires IsRanked<R, E> && IsNormalizedRow<R, U>
inline constexpr std::optional<std::size_t> row_lookup_normalized_v =
    detail::row_lookup_normalized_impl_adapter<R, E, U>::value;

template <typename R, typename E, typename U>
requires IsRanked<R, E> && IsNormalizedRow<R, U>
inline constexpr bool row_elem_normalized_v = row_lookup_normalized_v<R, E, U>.has_value();

template <typename R, typename E, typename U>
requires IsRanked<R, E> && IsNormalizedRow<R, U> && row_elem_normalized_v<R, E, U>
inline constexpr std::size_t row_index_normalized_v = row_lookup_normalized_v<R, E, U>.value();

// Determine whether two normalized rows are equivalent.

namespace detail {

    template <typename R, typename... Es, typename... Fs>
    requires IsNormalizedPack<R, Es...> && IsNormalizedPack<R, Fs...>
    [[nodiscard]] consteval bool row_subset_normalized_impl(Row<Es...>, Row<Fs...>) {

        constexpr std::size_t sizeE = sizeof...(Es);
        constexpr std::array<std::size_t, sizeE> arrayE { rank_v<R, Es>... };

        constexpr std::size_t sizeF = sizeof...(Fs);
        constexpr std::array<std::size_t, sizeF> arrayF { rank_v<R, Fs>... };

        // std::ranges::includes uses std::ranges::less with the std::identity
        // projection.

        return std::ranges::includes(arrayF, arrayE);

    }

    template <typename R, typename... Es, typename... Fs>
    requires IsNormalizedPack<R, Es...> && IsNormalizedPack<R, Fs...>
    [[nodiscard]] consteval bool row_equiv_normalized_impl(Row<Es...>, Row<Fs...>) {

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
inline constexpr bool row_subset_normalized_v = detail::row_subset_normalized_impl<R>(U {}, V {});

template <typename R, typename U, typename V>
requires IsNormalizedRow<R, U> && IsNormalizedRow<R, V>
inline constexpr bool row_equiv_normalized_v = detail::row_equiv_normalized_impl<R>(U {}, V {});

template <typename R, typename U, typename V>
requires IsNormalizedRow<R, U> && IsNormalizedRow<R, V>
inline constexpr bool row_proper_subset_normalized_v = row_subset_normalized_v<R, U, V> && !row_equiv_normalized_v<R, U, V>;

// The status type is parameterized over normalized parameter packs.

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
    noexcept(is_nothrow_invocable_over_index_sequence_v<F, N>) {

        assert(n < N); /* precondition */

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
    noexcept(is_nothrow_invocable_over_index_sequence_v<F, N>) {
        return dispatch_linear_dense<N>(n, std::forward<F>(f));
    }

    template <typename F, typename... Es>
    inline constexpr bool is_nothrow_visitable_v =
        (std::is_nothrow_invocable_v<F, const Es&> && ...);

    template <typename Self, typename T>
    using const_preserving_pointer_t = std::conditional_t<std::is_const_v<std::remove_reference_t<Self>>, const T*, T*>;

    template <typename R, IsTriviallyStorable... Es>
    requires IsNormalizedPack<R, Es...>
    struct StatusImpl final {

        using TagType = std::size_t;
        using StorageType = Storage<Es...>;

        // Ensure class invariants are satisfied.

        static_assert(sizeof...(Es) > 0,
            "StatusImpl<R, Es...>: alternatives must be non-empty");

        static_assert((std::is_trivially_destructible_v<Es> && ...),
            "StatusImpl<R, Es...>: alternatives must be trivially destructible");

        static_assert((std::is_nothrow_copy_constructible_v<Es> && ...),
            "StatusImpl<R, Es...>: alternatives must be nothrow copy-constructible");

        static_assert((std::is_nothrow_move_constructible_v<Es> && ...),
            "StatusImpl<R, Es...>: alternatives must be nothrow move-constructible");

        // Construct a StatusImpl from an alternative.

        template <typename E, typename... Args>
        requires row_elem_normalized_v<R, E, Row<Es...>> &&
                 std::constructible_from<E, Args...>
        constexpr explicit StatusImpl(std::in_place_type_t<E>, Args&&... args)
        noexcept(std::is_nothrow_constructible_v<E, Args...>) :
            active_(row_index_normalized_v<R, E, Row<Es...>>),
            alternatives_(std::in_place_index<row_index_normalized_v<R, E, Row<Es...>>>, std::forward<Args>(args)...) {}

        // TODO: Remove the StatusImpl(E&&) constructor, which exists primarily
        // to enable the ResultImpl(Error<E>&&) and ResultImpl(const Error<E>&)
        // constructors. It overlaps with the copy and move constructors and is
        // ambiguous (and at most wrong) when E is the StatusImpl type itself.

        template <typename E>
        requires row_elem_normalized_v<R, std::remove_cvref_t<E>, Row<Es...>>
        constexpr StatusImpl(E&& e)
        noexcept(std::is_nothrow_constructible_v<std::remove_cvref_t<E>, E>) :
            StatusImpl(std::in_place_type<std::remove_cvref_t<E>>, std::forward<E>(e)) {}

        // Implicit widening constructor.

        template <IsTriviallyStorable... Fs>
        requires IsNormalizedPack<R, Fs...> &&
                row_proper_subset_normalized_v<R, Row<Fs...>, Row<Es...>>
        constexpr StatusImpl(const StatusImpl<R, Fs...>& other) noexcept /* triviality */ :
            active_{}, alternatives_{} /* dead initialization */ {

            other.visit([this]<typename E>(const E& e) -> void {
                constexpr std::size_t I = row_index_normalized_v<R, E, Row<Es...>>;
                this->active_ = I;
                storage_emplace<I>(this->alternatives_, e);
            });

        }

        // Return pointer to underlying storage by type.

        template <typename E, typename Self>
        constexpr auto get_if(this Self& self) noexcept -> const_preserving_pointer_t<Self, E> {
            if constexpr (row_elem_normalized_v<R, E, Row<Es...>>) {
                if (self.template holds<E>()) {
                    return std::addressof(storage_get<row_index_normalized_v<R, E, Row<Es...>>>(self.alternatives_));
                } else {
                    return nullptr;
                }
            } else {
                return nullptr;
            }
        }

        // Dispatch visitor to active member by index.

        template <typename F>
        constexpr decltype(auto) visit(F&& f) const
        noexcept(is_nothrow_visitable_v<F, Es...>) {
            return dispatch<sizeof...(Es)>(
                this->active_,
                [&]<std::size_t I>(std::integral_constant<std::size_t, I>) -> decltype(auto) {
                    return std::forward<F>(f)(storage_get<I>(this->alternatives_));
                }
            );
        }

        // Determine which alternative is active.

        template <typename E>
        [[nodiscard]] constexpr bool holds() const noexcept {
            if constexpr (row_elem_normalized_v<R, E, Row<Es...>>) {
                return this->active_ == row_index_normalized_v<R, E, Row<Es...>>;
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
        // tibility. The noexcept specifications are for documentary purposes.

        StatusImpl() = delete;

        StatusImpl(const StatusImpl& other) noexcept = default;
        StatusImpl& operator=(const StatusImpl&) noexcept = default;

        StatusImpl(StatusImpl&& other) noexcept = default;
        StatusImpl& operator=(StatusImpl&& other) noexcept = default;

        ~StatusImpl() noexcept = default;

    };

    template <typename M, typename U>
    struct status_impl_pack_adapter;

    template <typename M, IsTriviallyStorable... Es>
    requires IsRankedPack<M, Es...>
    struct status_impl_pack_adapter<M, Row<Es...>> : std::type_identity<StatusImpl<M, Es...>> {};

    // TODO: We probably want to define generic "IsErrorPack" and "IsErrorRow"
    // concepts that assert that a pack or row is normalized and trivially stor-
    // able; below we have trouble asserting IsTriviallyStorable on the row.

    template <typename M, typename U>
    requires IsNormalizedRow<M, U>
    using status_impl_pack_adapter_t = status_impl_pack_adapter<M, U>::type;

    // Normalized parameter pack by rank.

    template <typename R, typename... Es>
    requires IsRankedPack<R, Es...>
    consteval auto pack_normalize_indices() noexcept {

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

    // TODO: disable -Wc++26-extensions

    // #if defined(__cpp_pack_indexing) && __cpp_pack_indexing >= 202311L
    // template <std::size_t I, typename... Ts>
    // using type = Ts...[I];

    #if defined(__has_builtin) && __has_builtin(__type_pack_element)
    template <std::size_t I, typename... Ts>
    using pack_subscript_t = __type_pack_element<I, Ts...>;
    #else
    template <std::size_t I, typename... Ts>
    using pack_subscript_t = std::tuple_element_t<I, std::tuple<Ts...>>;
    #endif

    template <std::size_t I, typename U>
    struct row_subscript;

    template <std::size_t I, typename... Es>
    struct row_subscript<I, Row<Es...>> : std::type_identity<pack_subscript_t<I, Es...>> {};

    template <std::size_t I, IsRow U>
    using row_subscript_t = row_subscript<I, U>::type;

    template <typename R, typename... Es>
    requires IsRankedPack<R, Es...>
    inline constexpr auto pack_normalize_indices_v = pack_normalize_indices<R, Es...>();

    template <typename R, typename... Es>
    requires IsRankedPack<R, Es...>
    struct pack_normalize {

        static constexpr auto result = pack_normalize_indices_v<R, Es...>;
        static constexpr auto count = result.first;
        static constexpr auto indices = result.second;

        // pack_normalize<R, Es...>()::type is Row<Fs...> where Fs... is a sub-
        // set of Es... that is sorted and unique with respect to ranking func-
        // tion R::rank.

        using type = decltype(
            []<std::size_t... Is>(std::index_sequence<Is...>) {
                return []<std::size_t... Js>() -> Row<pack_subscript_t<Js, Es...>...> {
                    return {};
                }.template operator()<indices[Is]...>();
            }(std::make_index_sequence<count> {})
        );

    };

    template <typename R, typename... Es>
    requires IsRankedPack<R, Es...>
    using pack_normalize_t = pack_normalize<R, Es...>::type;

    template <typename R, typename U>
    struct row_normalize;

    template <typename R, typename... Es>
    struct row_normalize<R, Row<Es...>> : pack_normalize<R, Es...> {};

} // namespace detail

template <typename R, typename U>
requires IsRankedRow<R, U>
using row_normalize_t = detail::row_normalize<R, U>::type;

// Compute the union, intersection and difference of two or more rows.

enum class MergeOp : std::uint8_t { Union, Intersection, Difference };

namespace detail {

    // Track the source of an index when merging parameter packs.

    struct MergeStep {
        std::size_t index;
        std::size_t source;
    };

    // Bound the number of merge steps by the sum of the sizes of the parameter
    // packs.

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

            // If the frontier is empty then there are no more elements to con-
            // sider.

            if (min_rank_row == num_rows) {
                break;
            }

            // If the frontier is non-empty then min_rank is the minimum rank at
            // row min_rank_row and index min_rank_col.

            std::size_t min_rank_col = ranks_table_col[min_rank_row];

            // Count the number of occurrences of min_rank on the frontier. This
            // could be merged with the de-duplication pass but the performance
            // gain is minimal in practice and separate passes is much clearer.

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
                add_merge_step = min_rank_row == 0 && num_min_rank_frontier > 0;
            } else {
                std::unreachable();
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

        using type = decltype(
            []<std::size_t... Is>(std::index_sequence<Is...>) {
                return []<MergeStep... Ms>() ->Row<row_subscript_t<Ms.index, pack_subscript_t<Ms.source, Rows...>>...> {
                    return {};
                }.template operator()<merge_steps[Is]...>();
            }(std::make_index_sequence<merge_size> {})
        );

    };

} // namespace detail

template <typename M, typename... Rows>
using row_union_normalized_t = detail::merge_normalized_rows_linear<MergeOp::Union, M, Rows...>::type;

template <typename M, typename... Rows>
using row_intersection_normalized_t = detail::merge_normalized_rows_linear<MergeOp::Intersection, M, Rows...>::type;

template <typename M, typename... Rows>
using row_difference_t = detail::merge_normalized_rows_linear<MergeOp::Difference, M, Rows...>::type;

namespace detail {

    // Unpack a Row into a StatusImpl.

    template <typename R, typename U>
    struct status_row_adapter;

    template <typename R, typename... Es>
    struct status_row_adapter<R, Row<Es...>> : std::type_identity<StatusImpl<R, Es...>> {};

    template <typename R, typename Row>
    using status_row_adapter_t = typename status_row_adapter<R, Row>::type;

    // Normalizing constructor for StatusImpl.

    template <typename R, IsTriviallyStorable... Es>
    requires IsRankedPack<R, Es...>
    using Status = detail::status_row_adapter_t<R, detail::pack_normalize_t<R, Es...>>;

} // namespace detail

} // namespace varerr

#endif // VARERR_STATUS_HPP
