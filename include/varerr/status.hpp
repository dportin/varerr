#ifndef VARERR_STATUS_HPP
#define VARERR_STATUS_HPP

#include "utilities.hpp"
#include "storage.hpp"
#include "algebra.hpp"

#include <cassert>
#include <concepts>
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

// Determine whether a Row carries a trivially storable parameter pack.

namespace detail {

template <typename... Es>
struct is_trivially_storable_pack : std::bool_constant<(IsTriviallyStorable<Es> && ...)> {};

} // namespace detail

template <typename... Es>
concept IsTriviallyStorablePack = detail::is_trivially_storable_pack<Es...>::value;

template <typename U>
concept IsTriviallyStorableRow = IsRow<U> && pack_apply_v<bind_meta_adapter<detail::is_trivially_storable_pack>, U>;

namespace detail {

// Determine whether a function is nothrow invocable over an index sequence.

template <typename F, std::size_t N>
inline constexpr bool is_nothrow_invocable_over_index_sequence_v =
    []<std::size_t... Is>(std::index_sequence<Is...>) -> bool {
        return (std::is_nothrow_invocable_v<F, std::integral_constant<std::size_t, Is>> && ...);
    }(std::make_index_sequence<N> {});

// Dispatch a function F with the index of the active alternative. Note that F
// may contain forwarded state and thus must be invoked exactly once. Consider
// switching to binary search or jump table when the number of alternatives is
// large. The current implementation simulates a jump table using an unrolled
// chain of constexpr conditionals.

template <std::size_t N, typename F>
requires (N > 0)
[[nodiscard]] constexpr decltype(auto) dispatch_linear_dense(std::size_t n, F&& f)
noexcept(is_nothrow_invocable_over_index_sequence_v<F, N>) {

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
noexcept(is_nothrow_invocable_over_index_sequence_v<F, N>) {
    return dispatch_linear_dense<N>(n, std::forward<F>(f));
}

} // namespace detail

// The discriminator has the smallest unsigned integral type that discriminates
// between N alternatives. The returned std::uint_leastN_t types are unconditio-
// nally present.

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

// A visitor V is valid with respect to a parameter pack Es if V is invocable at
// and has a uniform return type for all alternatives E in Es. The concepts tra-
// ck the constness and value category of the Self parameter (so that a visitor
// accepting E& binds to an lvalue while one accepting E&& binds to an rvalue).

template <typename Self, typename E>
using visitor_argument_t = decltype(std::forward_like<Self>(std::declval<E&>()));

template <typename Self, typename V, typename E>
using visitor_invoke_result_t = std::invoke_result_t<V, visitor_argument_t<Self, E>>;

template <typename Self, typename V, typename... Es>
concept IsVisitorInvocableLike = (std::is_invocable_v<V, visitor_argument_t<Self, Es>> && ...);

template <typename Self, typename V, typename... Es>
concept IsVisitorUniformLike = IsVisitorInvocableLike<Self, V, Es...> && is_uniform_v<visitor_invoke_result_t<Self, V, Es>...>;

template <typename Self, typename V, typename E>
concept IsVisitorNothrowInvocableWithLike = std::is_nothrow_invocable_v<V, visitor_argument_t<Self, E>>;

// The BasicStatus class is parameterized by a normalized row of trivially stor-
// able types with nothrow copy and move constructors. Trivial storability impl-
// ies that every alternative is cvref-unqualified. Nothrow copy and move const-
// ructibility make the widening constructor unconditionally noexcept.

template <typename M, IsTriviallyStorable... Es>
requires IsNormalizedPack<M, Es...>
struct BasicStatus final {

    private:

    // DefaultType must be defined prior to the default constructor's requires
    // clause.

    using DiscrimType = status_discriminator_t<sizeof...(Es)>;
    using StorageType = detail::Storage<Es...>;
    using DefaultType = detail::pack_subscript_t<0, Es...>;

    public:

    // The primary template must have a non-empty parameter pack. The assertion
    // is vacuous because the empty case is handled by the BasicStatus<M> speci-
    // alization. It is retained for documentation.

    static_assert(sizeof...(Es) > 0,
        "BasicStatus: alternatives must be non-empty");

    // This class inherits trivial copyability and destructibility from its sto-
    // rage. The remaining class invariants ensure that the copy and move const-
    // ructors are unconditionally noexcept.

    static_assert((std::is_trivially_copyable_v<Es> && ...),
        "BasicStatus: alternatives must be trivially copyable");

    static_assert((std::is_trivially_destructible_v<Es> && ...),
        "BasicStatus: alternatives must be trivially destructible");

    static_assert((std::is_nothrow_copy_constructible_v<Es> && ...),
        "BasicStatus: alternatives must be nothrow copy-constructible");

    static_assert((std::is_nothrow_move_constructible_v<Es> && ...),
        "BasicStatus: alternatives must be nothrow move-constructible");

    // The default constructor default-constructs the first alternative.

    constexpr BasicStatus()
    noexcept(std::is_nothrow_default_constructible_v<DefaultType>)
    requires std::is_default_constructible_v<DefaultType> :
        discrim_ { static_cast<DiscrimType>(0) },
        storage_ { std::in_place_index<0> } {}

    // Construct a BasicStatus from an alternative.

    template <typename E, typename... Args>
    requires IsTriviallyStorable<E> &&
             row_elem_normalized_v<M, E, Row<Es...>> &&
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
    noexcept(std::is_nothrow_constructible_v<E, E>) :
        BasicStatus(std::in_place_type<E>, std::forward<E>(e)) {}

    // The widening constructor leaves the class members deliberately uninitial-
    // ized before invoking the visitor. P1331R2 permits uninitialized class me-
    // mbers in constexpr contexts provided that the uninitialized class members
    // are not read. The visitor writes each member before it returns. The class
    // invariants make the constructor unconditionally noexcept.

    // The parameter is a const reference to BasicStatus<M, Fs...> rather than a
    // forwarding reference since: (a) there are no move semantics as the alter-
    // natives are trivially copyable; and (b) the forwarding pattern would req-
    // uire destructuring and checking the universe and row for consistency.

    template <IsTriviallyStorable... Fs>
    requires IsNonEmptyRow<Row<Fs...>> &&
             IsNormalizedRow<M, Row<Fs...>> &&
             row_proper_subset_normalized_v<M, Row<Fs...>, Row<Es...>>
    constexpr BasicStatus(const BasicStatus<M, Fs...>& other) noexcept {
        other.visit([this]<typename E>(const E& e) -> void {
            constexpr std::size_t I = row_index_normalized_v<M, E, Row<Es...>>;
            this->discrim_ = static_cast<DiscrimType>(I);
            detail::storage_emplace<I>(this->storage_, e); /* copy */
        });
    }

    // Determine whether E is the active alternative.

    template <typename E>
    requires IsTriviallyStorable<E> &&
             row_elem_normalized_v<M, E, Row<Es...>>
    [[nodiscard]] constexpr bool holds() const noexcept {
        return this->discrim_ == static_cast<DiscrimType>(row_index_normalized_v<M, E, Row<Es...>>);
    }

    // Return the index of the underlying storage for the active alternative.

    [[nodiscard]] constexpr std::size_t index() const noexcept {
        return static_cast<std::size_t>(this->discrim_);
    }

    // The accessors are constrained to non-volatile lvalue references: volatile
    // is prohibited because the discriminator and active member must be consis-
    // tent and the class provides no internal synchronization; rvalue referenc-
    // es are prohibited to prevent dangling references and pointers.

    // Return a pointer to the underlying storage for alternative E if E is the
    // active alternative. Returns nullptr if E is not the active alternative.

    template <typename E, typename Self>
    requires IsTriviallyStorable<E> &&
             IsNonVolatileLValueReference<Self> &&
             row_elem_normalized_v<M, E, Row<Es...>>
    // NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward)
    [[nodiscard]] constexpr transfer_const_t<Self, E>* get_if(this Self&& self) noexcept  {
        if (self.template holds<E>()) {
            return std::addressof(detail::storage_get<row_index_normalized_v<M, E, Row<Es...>>>(self.storage_));
        }
        return nullptr;
    }

    // Return a reference to the underlying storage for alternative E if E is
    // the active alternative.

    template <typename E, typename Self>
    requires IsTriviallyStorable<E> &&
             IsNonVolatileLValueReference<Self> &&
             row_elem_normalized_v<M, E, Row<Es...>>
    // NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward)
    [[nodiscard]] constexpr transfer_const_t<Self, E>& get(this Self&& self) noexcept {
        auto pointer = self.template get_if<E>();
        assert(pointer && "BasicStatus::get: alternative not active");
        return *pointer;
    }

    // Dispatch a visitor F to the active member by index. The visit method re-
    // jects volatile but accepts rvalue references. The noexcept specification
    // asserts nothrow invocability over the entire error row and is thus more
    // conservative than necessary.

    template <typename Self, typename F>
    requires IsNonVolatile<Self> &&
             IsVisitorUniformLike<Self, F, Es...>
    constexpr decltype(auto) visit(this Self&& self, F&& f)
    noexcept((IsVisitorNothrowInvocableWithLike<Self, F, Es> && ...)) {
        return detail::dispatch<sizeof...(Es)>(
            self.discrim_,
            [&]<std::size_t I>(std::integral_constant<std::size_t, I>)
            noexcept(IsVisitorNothrowInvocableWithLike<Self, F, detail::pack_subscript_t<I, Es...>>)
                -> decltype(auto) {
                return std::forward<F>(f)(detail::storage_get<I>(std::forward<Self>(self).storage_));
            }
        );
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

// Destructure BasicStatus into its components.

namespace detail {

template <typename S>
struct status_traits;

template <typename M, typename... Es>
struct status_traits<BasicStatus<M, Es...>> {
    using UniverseType = M;
    using RowType = Row<Es...>;
};

template <typename S>
constexpr bool is_status_exact_v = false;

template <typename M, typename... Es>
constexpr bool is_status_exact_v<BasicStatus<M, Es...>> = true;

} // namespace detail

template <typename S>
concept IsStatus = detail::is_status_exact_v<std::remove_cvref_t<S>>;

template <IsStatus S>
using status_row_t = detail::status_traits<std::remove_cvref_t<S>>::RowType;

template <IsStatus S>
using status_universe_t = detail::status_traits<std::remove_cvref_t<S>>::UniverseType;

template <IsStatus S, std::size_t I>
using status_alternative_t = detail::row_subscript_t<I, status_row_t<S>>;

// Construct a BasicStatus from a normalized or non-normalized parameter pack.

template <typename M, typename... Es>
requires IsTriviallyStorablePack<Es...> &&
         IsNormalizedPack<M, Es...>
using status_from_normalized_pack_t = BasicStatus<M, Es...>;

template <typename M, typename... Es>
requires IsTriviallyStorablePack<Es...> &&
         IsRankedPack<M, Es...>
using status_from_pack_t = pack_apply_t<bind_lift_adapter<BasicStatus, M>, pack_normalize_t<M, Es...>>;

// Construct a BasicStatus from a normalized or non-normalized Row.

template <typename M, typename U>
requires IsRow<U> &&
         IsTriviallyStorableRow<U> &&
         IsNormalizedRow<M, U>
using status_from_normalized_row_t = pack_apply_t<bind_lift_adapter<BasicStatus, M>, U>;

template <typename M, typename U>
requires IsRow<U> &&
         IsTriviallyStorableRow<U> &&
         IsRankedRow<M, U>
using status_from_row_t = status_from_normalized_row_t<M, row_normalize_t<M, U>>;

// The normalizing constructor is an alias for BasicStatus.

template <typename M, typename... Es>
requires IsTriviallyStorablePack<Es...> &&
         IsRankedPack<M, Es...>
using Status = status_from_pack_t<M, Es...>;

} // namespace varerr

#endif // VARERR_STATUS_HPP
