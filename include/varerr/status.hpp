#ifndef VARERR_STATUS_HPP
#define VARERR_STATUS_HPP

#include "storage.hpp"
#include "algebra.hpp"

#include <concepts>
#include <cassert>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

// This file implements the variadic error type which forms the unexpected
// branch of the main result type.

namespace varerr {

namespace detail {

template <typename F, typename Seq>
inline constexpr bool is_nothrow_invocable_over_sequence_v = false;

template <typename F, std::size_t... Is>
inline constexpr bool is_nothrow_invocable_over_sequence_v<F, std::index_sequence<Is...>> =
    (std::is_nothrow_invocable_v<F, std::integral_constant<std::size_t, Is>> && ...);

template <typename F, std::size_t N>
inline constexpr bool is_nothrow_invocable_over_index_sequence_v =
    is_nothrow_invocable_over_sequence_v<F, std::make_index_sequence<N>>;

// Naive implementation of compile-time switch over indices. Consider replacing
// with jump table.

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

} // namespace detail

// template <typename F, typename Self, typename... Es>
// inline constexpr bool is_nothrow_visitable_v =
//     (std::is_nothrow_invocable_v<F, decltype(std::forward_like<Self>(std::declval<Es&>()))> && ...);

// More conservative than necessary but easier to reason about. The loss of pre-
// cision matters only for visitors that are nothrow-invocable for some but not
// all qualifications of a trivially copyable argument.

template <typename F, typename... Es>
inline constexpr bool is_nothrow_visitable_v =
    (std::is_nothrow_invocable_v<F, Es&> && ...) &&
    (std::is_nothrow_invocable_v<F, const Es&> && ...) &&
    (std::is_nothrow_invocable_v<F, Es&&> && ...) &&
    (std::is_nothrow_invocable_v<F, const Es&&> && ...);

template <typename Self, typename T>
using const_preserving_pointer_t = std::conditional_t<std::is_const_v<std::remove_reference_t<Self>>, const T*, T*>;

template <typename R, IsTriviallyStorable... Es>
requires IsNormalizedPack<R, Es...>
struct BasicStatus final {

    // The status class inherits trivial copyability and destructibility from
    // the storage class. The remaining class invariants must be satisfied for
    // the copy and move constructors to be unconditionally noexcept.

    static_assert(sizeof...(Es) > 0,
        "BasicStatus<M, Es...>: alternatives must be non-empty");

    static_assert((std::is_trivially_copyable_v<Es> && ...),
        "BasicStatus<M, Es...>: alternatives must be trivially copyable");

    static_assert((std::is_trivially_destructible_v<Es> && ...),
        "BasicStatus<M, Es...>: alternatives must be trivially destructible");

    static_assert((std::is_nothrow_copy_constructible_v<Es> && ...),
        "BasicStatus<M, Es...>: alternatives must be nothrow copy-constructible");

    static_assert((std::is_nothrow_move_constructible_v<Es> && ...),
        "BasicStatus<M, Es...>: alternatives must be nothrow move-constructible");

    // Construct a BasicStatus from an alternative.

    template <typename E, typename... Args>
    requires row_elem_normalized_v<R, E, Row<Es...>> &&
             std::constructible_from<E, Args...>
    constexpr explicit BasicStatus(std::in_place_type_t<E>, Args&&... args)
    noexcept(std::is_nothrow_constructible_v<E, Args...>) :
        active_(row_index_normalized_v<R, E, Row<Es...>>),
        alternatives_(std::in_place_index<row_index_normalized_v<R, E, Row<Es...>>>, std::forward<Args>(args)...) {}

    // TODO: Remove the forwarding constructor (which exists primarily to enab-
    // le the BasicResult(Error<E>&&) and BasicResult(const Error<E>&) construct-
    // ors. It overlaps with the copy and move constructors and is ambiguous -
    // and at most wrong - when E is the BasicStatus type itself.

    template <typename E>
    requires row_elem_normalized_v<R, std::remove_cvref_t<E>, Row<Es...>>
    constexpr BasicStatus(E&& e)
    noexcept(std::is_nothrow_constructible_v<std::remove_cvref_t<E>, E>) :
        BasicStatus(std::in_place_type<std::remove_cvref_t<E>>, std::forward<E>(e)) {}

    // Implicit widening constructor (row subsumption).

    template <IsTriviallyStorable... Fs>
    requires IsNormalizedPack<R, Fs...> &&
            row_proper_subset_normalized_v<R, Row<Fs...>, Row<Es...>>
    constexpr BasicStatus(const BasicStatus<R, Fs...>& other) noexcept /* triviality */ :
        active_{}, alternatives_{} /* dead initialization */ {

        other.visit([this]<typename E>(const E& e) -> void {
            constexpr std::size_t I = row_index_normalized_v<R, E, Row<Es...>>;
            this->active_ = I;
            storage_emplace<I>(this->alternatives_, e);
        });

    }

    // Return pointer to underlying storage by type.

    template <typename E, typename Self>
    constexpr const_preserving_pointer_t<Self, E> get_if(this Self& self) noexcept  {
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


    template <typename Self, typename F>
    constexpr decltype(auto) visit(this Self&& self, F&& f)
    noexcept(is_nothrow_visitable_v<F, Es...>) {
    // noexcept((std::is_nothrow_invocable_v<F, decltype(std::forward_like<Self>(std::declval<Es&>()))> && ...)) {
        return detail::dispatch<sizeof...(Es)>(
            self.active_,
            [&]<std::size_t I>(std::integral_constant<std::size_t, I>) -> decltype(auto) {
                return std::forward<F>(f)(storage_get<I>(std::forward<Self>(self).alternatives_));
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

    using TagType = std::size_t;
    using StorageType = detail::Storage<Es...>;

    TagType active_;
    StorageType alternatives_;

};

template <typename R>
struct BasicStatus<R> final {

    // BasicStatus<R> is uninhabited: no value of this type should exist because
    // the empty row has no alternatives. The copy and move constructors are de-
    // faulted only because std::expected expects copy constructibility. The no-
    // except specifications are for documentary purposes.

    BasicStatus() = delete;

    BasicStatus(const BasicStatus& other) noexcept = default;
    BasicStatus& operator=(const BasicStatus&) noexcept = default;

    BasicStatus(BasicStatus&& other) noexcept = default;
    BasicStatus& operator=(BasicStatus&& other) noexcept = default;

    ~BasicStatus() noexcept = default;

};

namespace detail {

// Unpack a Row into a BasicStatus.

template <typename M, typename U>
struct basic_status_row_adapter;

template <typename M, typename... Es>
requires IsNormalizedPack<M, Es...>
struct basic_status_row_adapter<M, Row<Es...>> : std::type_identity<BasicStatus<M, Es...>> {};

template <typename M, typename U>
requires IsNormalizedRow<M, U>
using basic_status_row_adapter_t = typename basic_status_row_adapter<M, U>::type;

} // namespace detail

// Normalizing constructor for BasicStatus.

template <typename R, IsTriviallyStorable... Es>
requires IsRankedPack<R, Es...>
using Status = detail::basic_status_row_adapter_t<R, pack_normalize_t<R, Es...>>;

} // namespace varerr

#endif // VARERR_STATUS_HPP
