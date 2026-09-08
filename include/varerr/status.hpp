#ifndef VARERR_STATUS_HPP
#define VARERR_STATUS_HPP

#include "utilities.hpp"
#include "storage.hpp"
#include "algebra.hpp"

#include <concepts>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <type_traits>
#include <utility>


// This file implements the variadic error type which forms the unexpected bran-
// ch of the main result type. The status type inherits trivial copyability and
// destructibility from its backing storage. It further requires that the alter-
// natives be nothrow copy and move constructible to guarantee that its copy and
// move constructors are unconditionally noexcept.

namespace varerr {

namespace detail {

// Determine whether a function is nothrow invocable over an index sequence.

template <typename F, typename Is>
inline constexpr bool is_nothrow_invocable_over_index_sequence_v = false;

template <typename F, std::size_t... Is>
inline constexpr bool is_nothrow_invocable_over_index_sequence_v<F, std::index_sequence<Is...>> =
    (std::is_nothrow_invocable_v<F, std::integral_constant<std::size_t, Is>> && ...);

template <typename F, std::size_t N>
inline constexpr bool is_nothrow_invocable_upto_index_v =
    is_nothrow_invocable_over_index_sequence_v<F, std::make_index_sequence<N>>;

// Dispatch a function with the index of the active alternative. Consider using
// a binary search or jump table when the number of alternatives is large to im-
// prove performance. The current implementation simulates a jump table using a
// compile-time unrolled conditional chain.

template <std::size_t N, typename F>
requires (N > 0)
[[nodiscard]] constexpr decltype(auto) dispatch_linear_dense(std::size_t n, F&& f)
noexcept(is_nothrow_invocable_upto_index_v<F, N>) {

    assert(n < N);

    return [&]<std::size_t I>(this auto&& self) -> decltype(auto) {
        if constexpr (I + 1 == N) {
            return std::forward<F>(f)(std::integral_constant<std::size_t, I> {});
        } else {
            if (n == I) {
                return std::forward<F>(f)(std::integral_constant<std::size_t, I> {});
            } else {
                return self.template operator()<I + 1>();
            }
        }
    }.template operator()<0>();
}

template <std::size_t N, typename F>
requires (N > 0)
[[nodiscard]] constexpr decltype(auto) dispatch(std::size_t n, F&& f)
noexcept(is_nothrow_invocable_upto_index_v<F, N>) {
    return dispatch_linear_dense<N>(n, std::forward<F>(f));
}

} // namespace detail

// Determine whether a function is nothrow invocable for every cref-qualified
// version of its arguments. This is more conservative than necessary but the
// loss of precision matters only for visitors that are nothrow invocable for
// some but not all cref-qualifications of their trivially copyable arguments.

template <typename F, typename... Es>
inline constexpr bool is_nothrow_visitable_v =
    (std::is_nothrow_invocable_v<F, Es&> && ...) &&
    (std::is_nothrow_invocable_v<F, const Es&> && ...) &&
    (std::is_nothrow_invocable_v<F, Es&&> && ...) &&
    (std::is_nothrow_invocable_v<F, const Es&&> && ...);

// Determine the smallest unsigned integral type that discriminates between N
// alternatives. The returned std::uint_leastN_t are unconditionally present.

namespace detail {

template <std::size_t N>
consteval auto status_discriminator_impl() {

    constexpr std::size_t K = N > 0 ? N - 1 : 0;

    static_assert(K <= std::numeric_limits<std::uint_least64_t>::max(),
        "status_discriminator: number of alternatives not representable in std::uint_least64_t");

    if constexpr (K <= std::numeric_limits<std::uint_least8_t>::max()) {
        return std::type_identity<std::uint_least8_t> {};
    } else if constexpr (K <= std::numeric_limits<std::uint_least16_t>::max()) {
        return std::type_identity<std::uint_least16_t> {};
    } else if constexpr (K <= std::numeric_limits<std::uint_least32_t>::max()) {
        return std::type_identity<std::uint_least32_t> {};
    } else if constexpr (K <= std::numeric_limits<std::uint_least64_t>::max()) {
        return std::type_identity<std::uint_least64_t> {};
    } else {
        std::unreachable();
    }

}

} // namespace detail

template <std::size_t N>
using status_discriminator_t = decltype(detail::status_discriminator_impl<N>())::type;

template <typename M, IsTriviallyStorable... Es>
requires IsNormalizedPack<M, Es...>
struct BasicStatus final {

    private:

    using DiscrimType = status_discriminator_t<sizeof...(Es)>;
    using StorageType = detail::Storage<Es...>;
    using DefaultType = detail::pack_subscript_t<0, Es...>;

    public:

    // This class inherits trivial copyability and destructibility from its sto-
    // rage. The sizeof invariant is unreachable but retained for documentation.
    // The remaining class invariants ensure that the copy and move constructors
    // are unconditionally noexcept.

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


    constexpr BasicStatus()
    noexcept(std::is_nothrow_default_constructible_v<DefaultType>)
    requires std::is_default_constructible_v<DefaultType> :
        discrim_ { static_cast<DiscrimType>(0) },
        storage_ { std::in_place_index<0>, DefaultType {} } {}

    // Construct a BasicStatus from an alternative.

    template <typename E, typename... Args>
    requires row_elem_normalized_v<M, E, Row<Es...>> &&
             std::constructible_from<E, Args...>
    constexpr explicit BasicStatus(std::in_place_type_t<E>, Args&&... args)
    noexcept(std::is_nothrow_constructible_v<E, Args...>) :
        discrim_ { static_cast<DiscrimType>(row_index_normalized_v<M, E, Row<Es...>>) },
        storage_ { std::in_place_index<row_index_normalized_v<M, E, Row<Es...>>>, std::forward<Args>(args)... } {}

    // TODO: Remove the forwarding constructor (which exists primarily to enab-
    // le the BasicResult(Error<E>&&) and BasicResult(const Error<E>&) construct-
    // ors. It overlaps with the copy and move constructors and is ambiguous -
    // and at most wrong - when E is the BasicStatus type itself.

    template <typename E>
    requires row_elem_normalized_v<M, std::remove_cvref_t<E>, Row<Es...>>
    constexpr BasicStatus(E&& e)
    noexcept(std::is_nothrow_constructible_v<std::remove_cvref_t<E>, E>) :
        BasicStatus(std::in_place_type<std::remove_cvref_t<E>>, std::forward<E>(e)) {}

    // The widening constructor deliberately leaves the class members uninitial-
    // ized before invoking the visitor. P1331R2 permits uninitialized class me-
    // mbers in constexpr contexts provided that the uninitialized class members
    // are not read. The visitor writes each member once before it returns. Note
    // that the alternatives are default-initialized regardless. The constructor
    // is unconditionally noexcept.

    template <IsTriviallyStorable... Fs>
    requires IsNormalizedPack<M, Fs...> &&
             row_proper_subset_normalized_v<M, Row<Fs...>, Row<Es...>>
    constexpr BasicStatus(const BasicStatus<M, Fs...>& other) noexcept {
        other.visit([this]<typename E>(const E& e) -> void {
            constexpr std::size_t I = row_index_normalized_v<M, E, Row<Es...>>;
            this->discrim_ = static_cast<DiscrimType>(I);
            detail::storage_emplace<I>(this->storage_, e);
        });
    }

    // Return a pointer to the underlying storage by alternative.

    template <typename E, typename Self>
    [[nodiscard]] constexpr transfer_const_t<Self, E>* get_if(this Self& self) noexcept  {
        if constexpr (row_elem_normalized_v<M, E, Row<Es...>>) {
            if (self.template holds<E>()) {
                return std::addressof(detail::storage_get<row_index_normalized_v<M, E, Row<Es...>>>(self.storage_));
            } else {
                return nullptr;
            }
        } else {
            return nullptr;
        }
    }

    // Dispatch a visitor to the active member by index.

    template <typename Self, typename F>
    constexpr decltype(auto) visit(this Self&& self, F&& f)
    noexcept(is_nothrow_visitable_v<F, Es...>) {
        return detail::dispatch<sizeof...(Es)>(
            self.discrim_,
            [&]<std::size_t I>(std::integral_constant<std::size_t, I>) -> decltype(auto) {
                return std::forward<F>(f)(detail::storage_get<I>(std::forward<Self>(self).storage_));
            }
        );
    }

    // Determine whether an alternative is active.

    template <typename E>
    [[nodiscard]] constexpr bool holds() const noexcept {
        if constexpr (row_elem_normalized_v<M, E, Row<Es...>>) {
            return this->discrim_ == static_cast<DiscrimType>(row_index_normalized_v<M, E, Row<Es...>>);
        } else {
            return false;
        }
    }

    private:

    DiscrimType discrim_;
    StorageType storage_;

};

template <typename M>
struct BasicStatus<M> final {

    // The BasicStatus<R> specialization is uninhabited. No value of this type
    // exists because the empty row has no alternatives. The copy and move con-
    // structors are defined only because std::expected requires copy construc-
    // tibility. The noexcept specifications are only for documentary purposes.

    BasicStatus() = delete;

    BasicStatus(const BasicStatus& other) noexcept = default;
    BasicStatus& operator=(const BasicStatus&) noexcept = default;

    BasicStatus(BasicStatus&& other) noexcept = default;
    BasicStatus& operator=(BasicStatus&& other) noexcept = default;

    ~BasicStatus() noexcept = default;

};

namespace detail {

template <typename M, typename U>
struct basic_status_row_adapter;

template <typename M, typename... Es>
requires IsNormalizedPack<M, Es...>
struct basic_status_row_adapter<M, Row<Es...>> : std::type_identity<BasicStatus<M, Es...>> {};

template <typename M, typename U>
requires IsNormalizedRow<M, U>
using basic_status_row_adapter_t = basic_status_row_adapter<M, U>::type;

} // namespace detail

// Construct a BasicStatus from a Row.

template <typename M, IsTriviallyStorable... Es>
requires IsRankedPack<M, Es...>
using Status = detail::basic_status_row_adapter_t<M, pack_normalize_t<M, Es...>>;

} // namespace varerr

#endif // VARERR_STATUS_HPP
