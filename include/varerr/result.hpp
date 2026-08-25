#ifndef VARERR_RESULT_HPP
#define VARERR_RESULT_HPP

#include "status.hpp"

#include <cassert>
#include <concepts>
#include <expected>
#include <type_traits>
#include <utility>

namespace varerr {

template <typename E>
struct Error {

    constexpr explicit Error(const E& e)
    noexcept(std::is_nothrow_copy_constructible_v<E>) :
        error_(e) {}

    constexpr explicit Error(E&& e)
    noexcept(std::is_nothrow_move_constructible_v<E>) :
        error_(std::move(e)) {}

    template <typename... Args>
    requires std::constructible_from<E, Args&&...>
    constexpr explicit Error(std::in_place_t, Args&&... args)
    noexcept(std::is_nothrow_constructible_v<E, Args&&...>) :
        error_(std::forward<Args>(args)...) {}

    [[nodiscard]] constexpr const E& unwrap() const & {
        return this->error_;
    }

    [[nodiscard]] constexpr E&& unwrap() && {
        return std::move(this->error_);
    }

    private:

    E error_;

};

// Deduces std::remove_cvref_t<E> from E&& since Error boxes the value.

template <typename E>
Error(E&&) -> Error<std::remove_cvref_t<E>>;

namespace detail {

    template <typename F, typename Self, typename T>
    struct forwarding_voidable_invoke_result;

    template <typename F, typename Self, typename T>
    requires (!std::is_void_v<T>)
    struct forwarding_voidable_invoke_result<F, Self, T> :
        std::invoke_result<F, decltype(std::forward_like<Self>(std::declval<T&>()))> {};

    template <typename F, typename Self>
    struct forwarding_voidable_invoke_result<F, Self, void> :
        std::invoke_result<F> {};

    template <typename F, typename Self, typename T>
    using forwarding_voidable_invoke_result_t = forwarding_voidable_invoke_result<F, Self, T>::type;

    // Destructure a ResultImpl into its components.

    template <typename M, typename T, IsTriviallyStorable... Es>
    requires IsNormalizedPack<M, Es...>
    struct ResultImpl;

    template <typename X>
    struct result_impl_traits;

    template <typename M, typename T, typename... Es>
    struct result_impl_traits<ResultImpl<M, T, Es...>> {

        using UniverseType = M;
        using ValueType = T;
        using RowType = Row<Es...>;

        template <typename R, typename U>
        struct rebind_row_adapter;

        template <typename R, typename... Fs>
        struct rebind_row_adapter<R, Row<Fs...>> : std::type_identity<ResultImpl<M, R, Fs...>> {};

        template <typename R, typename U>
        using rebind = rebind_row_adapter<R, U>::type;

    };

    // Determine whether a type is a ResultImpl.

    template <typename X>
    inline constexpr bool is_result_impl_exact_v = false;

    template <typename M, typename T, typename... Es>
    inline constexpr bool is_result_impl_exact_v<ResultImpl<M, T, Es...>> = true;

    template <typename X>
    inline constexpr bool is_result_impl_v = is_result_impl_exact_v<std::remove_cvref_t<X>>;

    template <typename X>
    concept IsResult = is_result_impl_v<X>;

    // Helpers for destructuring a ResultImpl.

    template <IsResult X>
    using result_universe_t = result_impl_traits<std::remove_cvref_t<X>>::UniverseType;

    template <IsResult X>
    using result_value_t = result_impl_traits<std::remove_cvref_t<X>>::ValueType;

    template <IsResult X>
    using result_row_t = result_impl_traits<std::remove_cvref_t<X>>::RowType;

    template <IsResult X, typename R, IsRow U>
    using result_rebind_t = result_impl_traits<std::remove_cvref_t<X>>::template rebind<R, U>;

    // Determine whether a handler is invocable at a point.

    template <typename Self, typename E>
    using handler_argument_t = decltype(std::forward_like<Self>(std::declval<E&>()));

    template <typename H, typename Self, typename E>
    using handler_invoke_result_t = std::invoke_result_t<H, handler_argument_t<Self, E>>;

    template <typename H, typename Self, typename E>
    inline constexpr bool is_handler_invocable_v = std::is_invocable_v<H, handler_argument_t<Self, E>>;

    // Determine whether a handler is valid at a point.

    template <typename H, typename Self, typename E>
    inline constexpr bool is_handler_branch_valid_invocable_v =
        is_handler_invocable_v<H, Self, E>;

    template <typename H, typename Self, typename E>
    inline constexpr bool is_handler_branch_valid_result_v =
        is_result_impl_v<std::remove_cvref_t<handler_invoke_result_t<H, Self, E>>>;

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

    template <typename H, typename Self, typename E>
    inline constexpr bool is_handler_branch_valid_v =
        is_handler_branch_valid_invocable_v<H, Self, E> &&
        is_handler_branch_valid_result_v<H, Self, E> &&
        is_handler_branch_valid_universe_v<H, Self, E> &&
        is_handler_branch_valid_value_v<H, Self, E>;

    // The main result type.

    template <typename M, typename T, IsTriviallyStorable... Es>
    requires IsNormalizedPack<M, Es...>
    struct ResultImpl final {

        using ValueType = T;
        using ErrorType = detail::StatusImpl<M, Es...>;
        using ResultType = std::expected<ValueType, ErrorType>;

        template <typename N, typename R, IsTriviallyStorable... Fs>
        requires IsNormalizedPack<N, Fs...>
        friend struct ResultImpl;

        // Construct a ResultImpl from a T.

        explicit constexpr ResultImpl(const T& r)
        noexcept(noexcept(ResultType(r))) :
            result_(r) {}

        explicit constexpr ResultImpl(T&& r)
        noexcept(noexcept(ResultType(r))) :
            result_(std::move(r)) {}

        // Construct a ResultImpl from an Error.

        // TODO: the Error<E> constructors depend on the StatusImpl(E&&) forwar-
        // ding constructor, which overlaps in some cases with the default copy
        // and move constructors. Consider a different design here.

        template <typename E>
        requires row_elem_normalized_v<M, E, Row<Es...>>
        constexpr ResultImpl(const Error<E>& e)
        noexcept(noexcept(ResultType(std::unexpected(std::declval<const E&>())))) /* TODO: noexcept(true) */ :
            result_(std::unexpected(e.unwrap())) {}

        template <typename E>
        requires row_elem_normalized_v<M, E, Row<Es...>>
        constexpr ResultImpl(Error<E>&& e)
        noexcept(noexcept(ResultType(std::unexpected(std::declval<E&&>())))) /* TODO: noexcept(true) */ :
            result_(std::unexpected(std::move(e).unwrap())) {}

        // Implicit widening constructor.

        template <IsTriviallyStorable... Fs>
        requires IsNormalizedPack<M, Fs...> &&
                 row_proper_subset_normalized_v<M, Row<Fs...>, Row<Es...>>
        constexpr ResultImpl(const ResultImpl<M, T, Fs...>& other)
        noexcept(noexcept(widen(other))) :
            result_(widen(other)) {}

        template <IsTriviallyStorable... Fs>
        requires IsNormalizedPack<M, Fs...> &&
                 row_proper_subset_normalized_v<M, Row<Fs...>, Row<Es...>>
        constexpr ResultImpl(ResultImpl<M, T, Fs...>&& other)
        noexcept(noexcept(widen(std::move(other)))) :
            result_(widen(std::move(other))) {}

        // Boolean observers

        [[nodiscard]] constexpr bool has_value() const noexcept {
            return this->result_.has_value();
        }

        [[nodiscard]] constexpr explicit operator bool() const noexcept {
            return this->has_value();
        }

        [[nodiscard]] constexpr bool has_error() const noexcept {
            return !this->has_value();
        }

        template <typename E>
        [[nodiscard]] constexpr bool holds_error() const noexcept {
            if constexpr (row_elem_normalized_v<M, E, Row<Es...>>) {
                return this->has_error() && this->status().template holds<E>();
            } else {
                return false;
            }
        }

        // Value accessors

        template <typename Self>
        // NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward)
        [[nodiscard]] constexpr auto&& value(this Self&& self) {
            assert(self.result_.has_value());
            return std::forward_like<Self>(*self.result_);
        }

        template <typename Self>
        [[nodiscard]] constexpr const_preserving_pointer_t<Self, T> value_if(this Self& self) noexcept {
            if (self.has_value()) {
                return std::addressof(*self.result_);
            } else {
                return nullptr;
            }
        }

        // Error accessors

        template <typename E, typename Self>
        // NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward)
        [[nodiscard]] constexpr auto&& error(this Self&& self) {
            static_assert(row_elem_normalized_v<M, E, Row<Es...>>);
            assert(self.template holds_error<E>());
            return std::forward_like<Self>(*self.status().template get_if<E>());
        }

        template <typename E, typename Self>
        [[nodiscard]] constexpr const_preserving_pointer_t<Self, E> error_if(this Self& self) noexcept {
            static_assert(row_elem_normalized_v<M, E, Row<Es...>>);
            return self.status().template get_if<E>();
        }

        // The transform (fmap) combinator.

        template <typename Self, typename F>
        [[nodiscard]] auto /* prvalue */ transform(this Self&& self, F&& f) {

            // transform :: Result<M, T, U> ->
            //              (T -> S) ->
            //              Result<M, S, V>

            using InvokeF = std::remove_cvref_t<forwarding_voidable_invoke_result_t<F, Self, T>>; /* decayed */
            using ResultF = ResultImpl<M, InvokeF, Es...>;

            if (self.has_error()) [[unlikely]] {
                return ResultF(std::unexpect, std::forward<Self>(self).status());
            }

            const auto invoke = [&f, &self]() -> decltype(auto) /* decayed */ {
                if constexpr (std::is_void_v<T>) {
                    return std::invoke(std::forward<F>(f));
                } else {
                    return std::invoke(std::forward<F>(f), std::forward<Self>(self).value());
                }
            };

            if constexpr (std::is_void_v<InvokeF>) {
                static_cast<void>(invoke()); return ResultF(std::in_place);
            } else {
                return ResultF(std::in_place, invoke());
            }

        }

        // The and_then (bind) combinator.

        template <typename Self, typename F>
        [[nodiscard]] auto /* prvalue */ and_then(this Self&& self, F&& f) {

            // and_then :: Result<M, T, U> ->
            //             T -> Result<M, S, V> ->
            //             Result<M, S, U + V>

            using InvokeF = std::remove_cvref_t<forwarding_voidable_invoke_result_t<F, Self, T>>; /* decayed */

            static_assert(is_result_impl_v<InvokeF>, "and_then: F must return a ResultImpl");
            static_assert(std::same_as<result_universe_t<InvokeF>, M>, "and_then: F must preserve the universe M");
            static_assert(is_normalized_row_v<M, result_row_t<InvokeF>>, "and_then: F must return a normalized error row");

            using ErrRowF = row_union_normalized_t<M, Row<Es...>, result_row_t<InvokeF>>;
            using StatusF = status_impl_pack_adapter_t<M, ErrRowF>;
            using ResultF = result_rebind_t<InvokeF, result_value_t<InvokeF>, ErrRowF>;

            if (self.has_error()) [[unlikely]] {
                return ResultF(std::unexpect, StatusF(std::forward<Self>(self).status()));
            }

            if constexpr (std::is_void_v<T>) {
                return ResultF(std::invoke(std::forward<F>(f)));
            } else {
                return ResultF(std::invoke(std::forward<F>(f), std::forward<Self>(self).value()));
            }

        }

        // The handle (and_then/bind on the error row) combinator.

        template <IsTriviallyStorable... Fs, typename Self, typename H>
        requires (sizeof...(Fs) > 0) && IsRankedPack<M, Fs...>
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

            using StatusH = status_impl_pack_adapter_t<M, ErrRowH>;
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
                    return ResultH(std::invoke(std::forward<H>(h), std::forward_like<Self>(f)));
                } else {
                    return ResultH(std::unexpect, StatusH(std::in_place_type<std::remove_cvref_t<F>>, std::forward_like<Self>(f)));
                }
            });

        }

        private:

        [[nodiscard]] const ErrorType& status() const & {
            assert(this->has_error());
            return this->result_.error();
        }

        [[nodiscard]] ErrorType& status() & {
            assert(this->has_error());
            return this->result_.error();
        }

        // Construct a ResultImpl from a std::expected.

        template <typename... Args>
        constexpr ResultImpl(std::in_place_t, Args&&... args)
        noexcept(noexcept(ResultType(std::in_place, std::forward<Args>(args)...)))
            : result_(std::in_place, std::forward<Args>(args)...) {}

        template <typename S>
        constexpr ResultImpl(std::unexpect_t, S&& status)
        noexcept(noexcept(ResultType(std::unexpect, std::forward<S>(status))))
            : result_(std::unexpect, std::forward<S>(status)) {}

        // Explicit widening helper

        template <typename Other> /* unconstrained */
        [[nodiscard]] static constexpr ResultType widen(Other&& other)
        noexcept(noexcept(ResultType(std::in_place, std::forward<Other>(other).value()))) {

            static_assert(is_result_impl_v<Other>);
            using ErrRow = result_row_t<std::remove_cvref_t<Other>>;

            if (other.has_value()) {
                return ResultType(std::in_place, std::forward<Other>(other).value());
            }

            if constexpr (row_size_v<ErrRow> > 0) {
                return ResultType(std::unexpect, StatusImpl<M, Es...>(std::forward<Other>(other).status())); /* noexcept */
            } else {
                std::unreachable();
            }

        }

        ResultType result_;

    };

    // Unpack a Row into a ResultImpl.

    template <typename M, typename T, typename U>
    struct result_row_adapter;

    template <typename M, typename T, typename... Es>
    struct result_row_adapter<M, T, Row<Es...>> : std::type_identity<ResultImpl<M, T, Es...>> {};

    template <typename M, typename T, typename U>
    using result_row_adapter_t = typename result_row_adapter<M, T, U>::type;

} // namespace detail

// TODO: The public-facing concept should probably be named "IsErrorRow" rather
// than "IsRankedPack" since that is an implementation detail. Should also have
// a concept for "IsUniverse" to prevent error cascades when "R" does not provi-
// de a rank function. Unfortunately we can't test injectivity.

template <typename M, typename T, IsTriviallyStorable... Es>
requires detail::IsRankedPack<M, Es...>
using Result = detail::result_row_adapter_t<M, T, detail::pack_normalize_t<M, Es...>>;

} // namespace varerr

#endif // VARERR_RESULT_HPP
