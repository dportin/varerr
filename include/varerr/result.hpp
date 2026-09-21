#ifndef VARERR_RESULT_HPP
#define VARERR_RESULT_HPP

#include "utilities.hpp"
#include "storage.hpp"
#include "algebra.hpp"
#include "status.hpp"

#include <cassert>
#include <concepts>
#include <expected>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>


// This file contains the implementation of the main result type.

namespace varerr {

// Forward a type T like Self.

namespace detail {

template <typename Self, typename T>
struct forward_argument_like : std::type_identity<decltype(std::forward_like<Self>(std::declval<T&>()))> {};

} // namespace detail

template <typename Self, typename T>
using forward_argument_like_t = detail::forward_argument_like<Self, T>::type;

// Forward a voidable type T like Self.

namespace detail {

template <typename Self, typename T>
struct forward_voidable_argument_like : std::type_identity<forward_argument_like_t<Self, T>> {};

template <typename Self>
struct forward_voidable_argument_like<Self, void> : std::type_identity<void> {};

} // namespace detail

template <typename Self, typename T>
using forward_voidable_argument_like_t = detail::forward_voidable_argument_like<Self, T>::type;

// Invoke a function F with a voidable T.

template <typename F, typename T>
using voidable_invoke_result_t = voidable_apply_t<bind_meta_adapter<std::invoke_result, F>, T>;

template <typename Self, typename F, typename T>
using voidable_invoke_result_like_t = voidable_invoke_result_t<F, forward_voidable_argument_like_t<Self, T>>;

// Determine whether a function F is invocable with a voidable T.

template <typename F, typename T>
inline constexpr bool is_voidable_invocable_v = voidable_apply_v<bind_meta_adapter<std::is_invocable, F>, T>;

template <typename Self, typename F, typename T>
inline constexpr bool is_voidable_invocable_like_v = is_voidable_invocable_v<F, forward_voidable_argument_like_t<Self, T>>;

template <typename F, typename T>
concept IsVoidableInvocable = is_voidable_invocable_v<F, T>;

template <typename Self, typename F, typename T>
concept IsVoidableInvocableLike = is_voidable_invocable_like_v<Self, F, T>;

// Determine whether a function F is nothrow invocable with a voidable T.

template <typename F, typename T>
inline constexpr bool is_nothrow_voidable_invocable_v = voidable_apply_v<bind_meta_adapter<std::is_nothrow_invocable, F>, T>;

template <typename Self, typename F, typename T>
inline constexpr bool is_nothrow_voidable_invocable_like_v = is_nothrow_voidable_invocable_v<F, forward_voidable_argument_like_t<Self, T>>;

template <typename F, typename T>
concept IsNothrowVoidableInvocable = is_nothrow_voidable_invocable_v<F, T>;

template <typename Self, typename F, typename T>
concept IsNothrowVoidableInvocableLike = is_nothrow_voidable_invocable_like_v<Self, F, T>;

// Determine whether a voidable T is constructible from Args.

namespace detail {

template <typename T, typename... Args>
struct is_voidable_constructible : std::bool_constant<std::is_constructible_v<T, Args...>> {};

template <typename... Args>
struct is_voidable_constructible<void, Args...> : std::bool_constant<sizeof...(Args) == 0> {};

} // namespace detail

template <typename T, typename... Args>
inline constexpr bool is_voidable_constructible_v = detail::is_voidable_constructible<T, Args...>::value;

template <typename T, typename R>
inline constexpr bool is_voidable_constructible_from_v = voidable_apply_v<bind_meta_adapter<detail::is_voidable_constructible, T>, R>;

template <typename T>
inline constexpr bool is_voidable_constructible_from_self_v = is_voidable_constructible_from_v<T, T>;

template <typename Self, typename T, typename R>
inline constexpr bool is_voidable_constructible_from_like_v = is_voidable_constructible_from_v<T, forward_voidable_argument_like_t<Self, R>>;

template <typename Self, typename T>
inline constexpr bool is_voidable_constructible_from_self_like_v = is_voidable_constructible_from_like_v<Self, T, T>;

template <typename T, typename... Args>
concept IsVoidableConstructible = is_voidable_constructible_v<T, Args...>;

template <typename T, typename R>
concept IsVoidableConstructibleFrom = is_voidable_constructible_from_v<T, R>;

template <typename T>
concept IsVoidableConstructibleFromSelf = is_voidable_constructible_from_self_v<T>;

template <typename Self, typename T, typename R>
concept IsVoidableConstructibleFromLike = is_voidable_constructible_from_like_v<Self, T, R>;

template <typename Self, typename T>
concept IsVoidableConstructibleFromSelfLike = is_voidable_constructible_from_self_like_v<Self, T>;

// Determine whether a voidable T is nothrow constructible from Args.

namespace detail {

template <typename T, typename... Args>
struct is_nothrow_voidable_constructible : std::bool_constant<std::is_nothrow_constructible_v<T, Args...>> {};

template <typename... Args>
struct is_nothrow_voidable_constructible<void, Args...> : std::bool_constant<sizeof...(Args) == 0> {};

} // namespace detail

template <typename T, typename... Args>
inline constexpr bool is_nothrow_voidable_constructible_v = detail::is_nothrow_voidable_constructible<T, Args...>::value;

template <typename T, typename R>
inline constexpr bool is_nothrow_voidable_constructible_from_v = voidable_apply_v<bind_meta_adapter<detail::is_nothrow_voidable_constructible, T>, R>;

template <typename T>
inline constexpr bool is_nothrow_voidable_constructible_from_self_v = is_nothrow_voidable_constructible_from_v<T, T>;

template <typename Self, typename T, typename R>
inline constexpr bool is_nothrow_voidable_constructible_from_like_v = is_nothrow_voidable_constructible_from_v<T, forward_voidable_argument_like_t<Self, R>>;

template <typename Self, typename T>
inline constexpr bool is_nothrow_voidable_constructible_from_self_like_v = is_nothrow_voidable_constructible_from_like_v<Self, T, T>;

template <typename T, typename... Args>
concept IsNothrowVoidableConstructible = is_nothrow_voidable_constructible_v<T, Args...>;

template <typename T, typename R>
concept IsNothrowVoidableConstructibleFrom = is_nothrow_voidable_constructible_from_v<T, R>;

template <typename T>
concept IsNothrowVoidableConstructibleFromSelf = is_nothrow_voidable_constructible_from_self_v<T>;

template <typename Self, typename T, typename R>
concept IsNothrowVoidableConstructibleFromLike = is_nothrow_voidable_constructible_from_like_v<Self, T, R>;

template <typename Self, typename T>
concept IsNothrowVoidableConstructibleFromSelfLike = is_nothrow_voidable_constructible_from_self_like_v<Self, T>;

// Forward declaration of BasicResult.

template <typename M, typename T, IsTriviallyStorable... Es>
requires IsNormalizedPack<M, Es...>
struct BasicResult;

// Determine whether a type is a BasicResult.

namespace detail {

template <typename X>
inline constexpr bool is_result_exact_impl_v = false;

template <typename M, typename T, typename... Es>
inline constexpr bool is_result_exact_impl_v<BasicResult<M, T, Es...>> = true;

} // namespace detail

template <typename X>
inline constexpr bool is_result_v = detail::is_result_exact_impl_v<std::remove_cvref_t<X>>;

template <typename X>
concept IsResult = is_result_v<X>;

// Destructure a BasicResult into its components.

template <typename X>
struct result_impl_traits;

template <typename M, typename T, typename... Es>
struct result_impl_traits<BasicResult<M, T, Es...>> {

    using UniverseType = M;
    using ValueType = T;
    using RowType = Row<Es...>;
    using StatusType = BasicStatus<M, Es...>;

};

template <IsResult X>
using result_universe_t = result_impl_traits<std::remove_cvref_t<X>>::UniverseType;

template <IsResult X>
using result_value_t = result_impl_traits<std::remove_cvref_t<X>>::ValueType;

template <IsResult X>
using result_row_t = result_impl_traits<std::remove_cvref_t<X>>::RowType;

template <IsResult X>
using result_status_t = result_impl_traits<std::remove_cvref_t<X>>::StatusType;

template <IsResult X, std::size_t I>
using result_alternative_t = status_alternative_t<result_status_t<X>, I>;

// IWYU 0.26 (Clang 22.1.8) segfaults when a type alias declaration names a mem-
// ber template of a dependent type (although Clang accepts the same code). The
// workaround is to move rebind_row_adapter out of result_impl_traits.

namespace detail {

template <typename X, typename R, typename V>
struct result_rebind_adapter;

template <typename M, typename T, typename... Es, typename R, typename... Fs>
struct result_rebind_adapter<BasicResult<M, T, Es...>, R, Row<Fs...>> : std::type_identity<BasicResult<M, R, Fs...>> {};

} // namespace detail

template <IsResult X, typename R, IsRow V>
using result_rebind_t = detail::result_rebind_adapter<std::remove_cvref_t<X>, R, V>::type;

// TODO: REFACTOR HANDLER HELPERS

// Determine whether a handler is invocable and valid at a point.

template <typename Self, typename E>
using handler_argument_t = decltype(std::forward_like<Self>(std::declval<E&>()));

template <typename H, typename Self, typename E>
using handler_invoke_result_t = std::invoke_result_t<H, handler_argument_t<Self, E>>;

template <typename H, typename Self, typename E>
inline constexpr bool is_handler_branch_valid_invocable_v =
    std::is_invocable_v<H, handler_argument_t<Self, E>>;

template <typename H, typename Self, typename E>
inline constexpr bool is_handler_branch_valid_result_v =
    is_result_v<std::remove_cvref_t<handler_invoke_result_t<H, Self, E>>>;

template <typename H, typename Self, typename E>
inline constexpr bool is_handler_branch_valid_universe_v = std::same_as<
    result_universe_t<std::remove_cvref_t<Self>>,
    result_universe_t<std::remove_cvref_t<handler_invoke_result_t<H, Self, E>>>
>;

template <typename H, typename Self, typename E>
inline constexpr bool is_handler_branch_valid_value_v = std::same_as<
    result_value_t<std::remove_cvref_t<Self>>,
    result_value_t<std::remove_cvref_t<handler_invoke_result_t<H, Self, E>>>
>;

//

template <typename H, typename Self, typename E>
inline constexpr bool is_handler_branch_valid_v =
    is_handler_branch_valid_invocable_v<H, Self, E> &&
    is_handler_branch_valid_result_v<H, Self, E> &&
    is_handler_branch_valid_universe_v<H, Self, E> &&
    is_handler_branch_valid_value_v<H, Self, E>;

// Exception specification for the transform combinator.

template <typename Self, typename F, typename T>
inline constexpr bool is_nothrow_specification_transform_v =
    is_nothrow_voidable_invocable_like_v<Self, F, T> &&
    is_nothrow_voidable_constructible_from_v<
        std::remove_cvref_t<voidable_invoke_result_like_t<Self, F, T>>,
        voidable_invoke_result_like_t<Self, F, T>
    >;



// The main result type.

template <typename M, typename T, IsTriviallyStorable... Es>
requires IsNormalizedPack<M, Es...>
struct BasicResult final {

    // TODO: Assert trivial copy/move-constructibility in contsraints.

    private:

    using ValueType = T;
    using ErrorType = BasicStatus<M, Es...>;
    using ResultType = std::expected<ValueType, ErrorType>;

    public:

    template <typename N, typename R, IsTriviallyStorable... Fs>
    requires IsNormalizedPack<N, Fs...>
    friend struct BasicResult;

    // The following invariants are inherited from BasicStatus but repeated here
    // for documentation purposes.

    static_assert((std::is_trivially_copyable_v<Es> && ...),
        "BasicResult: alternatives must be trivially copyable");

    static_assert((std::is_trivially_destructible_v<Es> && ...),
        "BasicResult: alternatives must be trivially destructible");

    static_assert((std::is_nothrow_copy_constructible_v<Es> && ...),
        "BasicResult: alternatives must be nothrow copy-constructible");

    static_assert((std::is_nothrow_move_constructible_v<Es> && ...),
        "BasicResult: alternatives must be nothrow move-constructible");

    // Construct a BasicResult from a value T.

    template <typename... Args>
    requires IsVoidableConstructible<T, Args...>
    explicit constexpr BasicResult(std::in_place_t, Args&&... args)
    noexcept(is_nothrow_voidable_constructible_v<T, Args...>) :
        result_ { std::in_place, std::forward<Args>(args) ... } {}

    // Construct a BasicResult from an alternative E.

    template <typename E, typename... Args>
    requires IsElemExactInRow<M, Row<Es...>, E> &&
             std::is_constructible_v<E, Args...>
    explicit constexpr BasicResult(std::in_place_type_t<E>, Args&&... args)
    noexcept(std::is_nothrow_constructible_v<E, Args...>) :
        result_ { std::unexpect, ErrorType { std::in_place_type<E>, std::forward<Args>(args)... } } {}

    // The default constructor is equivalent to the in-place value constructor
    // with an empty argument list.

    explicit constexpr BasicResult()
    noexcept(is_nothrow_voidable_constructible_v<T>)
    requires IsVoidableConstructible<T> :
        result_ { std::in_place } {}

    // Construct a BasicResult from a narrower BasicResult via widening.

    template <typename Narrow>
    requires IsResult<Narrow> &&
             IsNonVolatile<Narrow> &&
             std::same_as<result_value_t<Narrow>, T> &&
             std::same_as<result_universe_t<Narrow>, M> &&
             IsVoidableConstructibleFromSelfLike<Narrow, T> &&
             IsNormalizedRow<M, result_row_t<Narrow>> &&
             row_proper_subset_normalized_v<M, result_row_t<Narrow>, Row<Es...>> &&
             IsRowExactInRow<M, Row<Es...>, result_row_t<Narrow>>
    constexpr BasicResult(Narrow&& narrow)
    noexcept(is_nothrow_voidable_constructible_from_self_like_v<Narrow, T>) :
        result_ { BasicResult::widen(std::forward<Narrow>(narrow)) } {}

    // Determine whether a BasicResult holds a value.

    [[nodiscard]] constexpr bool has_value() const noexcept {
        return this->result_.has_value();
    }

    [[nodiscard]] constexpr explicit operator bool() const noexcept {
        return this->has_value();
    }

    // Determine whether a BasicResult holds an error.

    [[nodiscard]] constexpr bool has_error() const noexcept {
        return !this->has_value();
    }

    template <typename E>
    requires IsElemExactInRow<M, Row<Es...>, E>
    [[nodiscard]] constexpr bool holds_error() const noexcept {
        return this->has_error() && this->status().template holds<E>();
    }

    // Return a pointer to the value or nullptr if the value branch is not ac-
    // tive. The deduced object parameter must be a non-volatile lvalue.

    template <typename Self>
    requires IsNonVoid<T> &&
             IsNonVolatileLValueReference<Self>
    // NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward)
    [[nodiscard]] constexpr transfer_const_t<Self, T>* value_if(this Self&& self) noexcept {
        if (self.has_value()) {
            return std::addressof(*self.result_);
        } else {
            return nullptr;
        }
    }

    // Return a reference to the value if the value branch is active. The deduc-
    // ed object parameter must be non-volatile. The asymmetry with BasicStatus
    // and value_if() is intentional since T has non-trivial move semantics.

    template <typename Self>
    requires IsNonVoid<T> &&
             IsNonVolatile<Self>
    // NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward)
    [[nodiscard]] constexpr forward_voidable_argument_like_t<Self, T> value(this Self&& self) noexcept {
        assert(self.has_value() && "BasicResult::value: value branch not active");
        return std::forward_like<Self>(*self.result_);
    }

    // Return a reference to the value if the error row is empty. The deduced
    // object parameter must be non-volatile. The assymetry with BasicStatus
    // and value_if() is intentional since T has non-trivial move semantics.

    template <typename Self>
    requires IsNonVoid<T> &&
             IsNonVolatile<Self> &&
             IsEmptyRow<Row<Es...>>
    // NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward)
    [[nodiscard]] constexpr forward_voidable_argument_like_t<Self, T> take(this Self&& self) noexcept {
        assert(self.has_value() && "BasicResult::take: value branch not active");
        return std::forward_like<Self>(*self.result_);
    }

    // Return a pointer to error alternative E or nullptr if the error branch is
    // not active or E is not the active error alternative. The deduced object
    // parameter must be a non-volatile lvalue.

    template <typename E, typename Self>
    requires IsNonVolatileLValueReference<Self> &&
             IsElemExactInRow<M, Row<Es...>, E>
    // NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward)
    [[nodiscard]] constexpr transfer_const_t<Self, E>* error_if(this Self&& self) noexcept {
        if (self.template holds_error<E>()) {
            return self.status().template get_if<E>();
        } else {
            return nullptr;
        }
    }

    // Return a reference to error alternative E if the error branch is active
    // and E is the active error alternative. The implicit object parameter is
    // constrained to non-volatile lvalue references because error() delegates
    // to BasicStatus::get_if(). The BasicStatus accessors are constrained to
    // lvalues to prevent dangling references and pointers.

    template <typename E, typename Self>
    requires IsNonVolatileLValueReference<Self> &&
             IsElemExactInRow<M, Row<Es...>, E>
    // NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward)
    [[nodiscard]] constexpr transfer_const_t<Self, E>& error(this Self&& self) noexcept {
        auto pointer = self.template error_if<E>();
        assert(pointer && "BasicResult::error: error branch not active");
        return *pointer;
    }

    // The transform combinator.

    template <typename Self, typename F>
    requires IsNonVolatile<Self> &&
             IsVoidableInvocableLike<Self, F, T>
    [[nodiscard]] constexpr auto /* prvalue */ transform(this Self&& self, F&& f)
    noexcept(is_nothrow_specification_transform_v<Self, F, T>) {

        // transform : Result<M, T, Es...> ->
        //             (T -> S) ->
        //             Result<M, S, Es...>

        using InvokeF = std::remove_cvref_t<voidable_invoke_result_like_t<Self, F, T>>;
        using ResultF = BasicResult<M, InvokeF, Es...>;

        // The constexpr guard is required to prevent the compiler from attempt-
        // ing to type-check a call to status() with an uninhabited BasicStatus.

        if constexpr (IsNonEmptyRow<Row<Es...>>) {
            if (self.has_error()) [[unlikely]] {
                return ResultF(std::unexpect, std::forward<Self>(self).status());
            }
        }

        const auto invoke = [&f, &self]() -> decltype(auto) {
            if constexpr (std::is_void_v<T>) {
                return std::invoke(std::forward<F>(f));
            } else {
                return std::invoke(std::forward<F>(f), std::forward<Self>(self).value());
            }
        };

        if constexpr (std::is_void_v<InvokeF>) {
            static_cast<void>(invoke());
            return ResultF(std::in_place);
        } else {
            return ResultF(std::in_place, invoke());
        }

    }

    // RESUME REFACTORING

    // The and_then (bind) combinator.

    template <typename Self, typename F>
    requires IsNonVolatile<Self>
    [[nodiscard]] auto /* prvalue */ and_then(this Self&& self, F&& f) {

        // and_then :: Result<M, T, U> ->
        //             T -> Result<M, S, V> ->
        //             Result<M, S, U + V>

        using InvokeF = std::remove_cvref_t<voidable_invoke_result_like_t<Self, F, T>>; /* decayed */

        static_assert(is_result_v<InvokeF>, "and_then: F must return a BasicResult");
        static_assert(std::same_as<result_universe_t<InvokeF>, M>, "and_then: F must preserve the universe M");
        static_assert(IsNormalizedRow<M, result_row_t<InvokeF>>, "and_then: F must return a normalized error row");

        using ErrRowF = row_union_normalized_t<M, Row<Es...>, result_row_t<InvokeF>>;
        using StatusF = status_from_normalized_row_t<M, ErrRowF>;
        using ResultF = result_rebind_t<InvokeF, result_value_t<InvokeF>, ErrRowF>;

        // The constexpr guard is required to prevent the compiler from attempt-
        // ing to type-check a call to the widening constructor with an uninhab-
        // ited BasicStatus.

        if constexpr (IsNonEmptyRow<Row<Es...>>) {
            if (self.has_error()) [[unlikely]] {
                return ResultF(std::unexpect, StatusF(std::forward<Self>(self).status()));
            }
        }

        if constexpr (std::is_void_v<T>) {
            return ResultF(std::invoke(std::forward<F>(f)));
        } else {
            return ResultF(std::invoke(std::forward<F>(f), std::forward<Self>(self).value()));
        }

    }

    // The handle (and_then/bind on the error row) combinator.

    template <IsTriviallyStorable... Fs, typename Self, typename H>
    requires IsNonVolatile<Self> &&
             IsNonEmptyRow<Row<Es...>> &&
             IsNonEmptyPack<Fs...> &&
             IsRankedPack<M, Fs...>
    [[nodiscard]] auto /* prvalue */ handle(this Self&& self, H&& h) {

        // If the handler handles a single alternative we have:
        //
        // handle :: Result<M, T, U> ->
        //           V_i in U -> Result<M, T, W_i> [N_i] ->
        //           Result<M, T, U \ {E_i} + W_i>
        //
        // In particular a handler can return a Result with a narrower, wid-
        // er or simply different error row. If the handler handles multiple
        // alternatives we have:
        //
        // handle :: Result<M, T, U> ->
        //           V <= U -> Result<M, T, W> [N]
        //           Result<M, R, U \ V + W>
        //
        // where V is the union of the V_i, W the union of the W_i and N the
        // sum of the N_i. In this case the value type must be uniform.

        static_assert(IsRankedPack<M, Fs...>,
            "handle: handled alternatives must be ranked");

        using HandledH = row_normalize_t<M, Row<Fs...>>;
        using RetainedH = row_difference_normalized_t<M, Row<Es...>, HandledH>;

        // Validate the error handler.

        static_assert((is_handler_branch_valid_invocable_v<std::remove_cvref_t<H>, Self, Fs> && ...),
            "handle: handler must be invocable for each declared alternative");

        static_assert((is_handler_branch_valid_result_v<std::remove_cvref_t<H>, Self, Fs> && ...),
            "handle: handler must return a result type");

        static_assert((is_handler_branch_valid_universe_v<std::remove_cvref_t<H>, Self, Fs> && ...),
            "handle: handler must preserve the universe parameter");

        static_assert((is_handler_branch_valid_value_v<std::remove_cvref_t<H>, Self, Fs> && ...),
            "handle: handler must preserve the value parameter");

        using ErrRowH = row_union_normalized_t<
            M,
            RetainedH,
            result_row_t<handler_invoke_result_t<std::remove_cvref_t<H>, Self, Fs>>...
        >;

        using StatusH = status_from_normalized_row_t<M, ErrRowH>;
        using ResultH = result_rebind_t<std::remove_cvref_t<Self>, T, ErrRowH>;

        if (self.has_value()) [[likely]] {
            if constexpr (std::is_void_v<T>) {
                return ResultH(std::in_place);
            } else {
                return ResultH(std::in_place, std::forward<Self>(self).value());
            }
        }

        // NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward)
        return std::forward<Self>(self).status().visit([&]<typename F>(F&& f) -> ResultH {
            if constexpr (row_elem_normalized_v<M, std::remove_cvref_t<F>, HandledH>) {
                return ResultH(std::invoke(std::forward<H>(h), std::forward<F>(f)));
            } else {
                return ResultH(std::unexpect, StatusH(std::in_place_type<std::remove_cvref_t<F>>, std::forward<F>(f)));
            }
        });

    }

    private:

    // The widening helper is unconditionally noexcept except when the value T
    // is present and non-void. In that case the noexcept specification tracks
    // the in-place construction of T from (the forwarded value of) itself.

    template <typename Narrow>
    [[nodiscard]] static constexpr ResultType widen(Narrow&& narrow)
    noexcept(is_nothrow_voidable_constructible_from_self_like_v<Narrow, T>) {

        if (narrow.has_value()) {
            if constexpr (std::is_void_v<T>) {
                return ResultType(std::in_place);
            } else {
                return ResultType(std::in_place, std::forward<Narrow>(narrow).value());
            }
        }

        if constexpr (IsNonEmptyRow<result_row_t<Narrow>>) {
            return ResultType(std::unexpect, ErrorType(std::forward<Narrow>(narrow).status()));
        } else {
            std::unreachable();
        }

    }

    template <typename Self>
    requires IsNonEmptyRow<Row<Es...>>
    [[nodiscard]] constexpr decltype(auto) status(this Self&& self) noexcept {
        assert(self.has_error());
        return std::forward<Self>(self).result_.error();
    }

    ResultType result_;

};

// Construct a BasicResult from a normalized or non-normalized parameter pack.

template <typename M, typename T, typename... Es>
requires IsTriviallyStorablePack<Es...> &&
         IsNormalizedPack<M, Es...>
using result_from_normalized_pack_t = BasicResult<M, T, Es...>;

template <typename M, typename T, typename... Es>
requires IsTriviallyStorablePack<Es...> &&
         IsRankedPack<M, Es...>
using result_from_pack_t = pack_apply_t<bind_lift_adapter<BasicResult, M, T>, pack_normalize_t<M, Es...>>;

// Construct a BasicResult from a normalized or non-normalized Row.

template <typename M, typename T, typename U>
requires IsRow<U> &&
         IsTriviallyStorableRow<U> &&
         IsNormalizedRow<M, U>
using result_from_normalized_row_t = pack_apply_t<bind_lift_adapter<BasicResult, M, T>, U>;

template <typename M, typename T, typename U>
requires IsRow<U> &&
         IsTriviallyStorableRow<U> &&
         IsRankedRow<M, U>
using result_from_row_t = result_from_normalized_row_t<M, T, row_normalize_t<M, U>>;

// The normalizing constructor is an alias for BasicResult.

template <typename M, typename T, typename... Es>
requires IsTriviallyStorablePack<Es...> &&
         IsRankedPack<M, Es...>
using Result = result_from_pack_t<M, T, Es...>;

} // namespace varerr

#endif // VARERR_RESULT_HPP
