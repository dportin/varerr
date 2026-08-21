#ifndef VARERR_RESULT_HPP
#define VARERR_RESULT_HPP

#include "status.hpp"

#include <cstddef>
#include <utility>
#include <expected>
#include <functional>
#include <concepts>
#include <type_traits>

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

    // template <typename W, typename E>
    // static constexpr bool is_nothrow_copy_constructible_from_unexpected_v =
    //     std::is_nothrow_copy_constructible_v<E> &&
    //     std::is_nothrow_constructible_v<W, std::unexpected<E>>;

    // template <typename W, typename E>
    // static constexpr bool is_nothrow_move_constructible_from_unexpected_v = 
    //     std::is_nothrow_move_constructible_v<E> /* redundant */ &&
    //     std::is_nothrow_constructible_v<W, std::unexpected<E>>;

    template <typename R, typename T, IsTriviallyStorable... Es>
    requires IsNormalizedPack<R, Es...>
    struct ResultImpl {

        using ValueType = T;
        using ErrorType = detail::StatusImpl<R, Es...>;
        using ResultType = std::expected<ValueType, ErrorType>;

        // Construct a ResultImpl from a T.

        // TODO

        // Construct a ResultImpl from an Error.

        template <typename E>
        requires row_elem_normalized_v<R, E, Row<Es...>> /* lifted */
        constexpr ResultImpl(const Error<E>& e)
        // noexcept(is_nothrow_copy_constructible_from_unexpected_v<ResultType, E>) :
        noexcept(noexcept(ResultType(std::unexpected(std::declval<const E&>())))) :
            result_(std::unexpected(e.unwrap())) {}
        
        template <typename E>
        requires row_elem_normalized_v<R, E, Row<Es...>> /* lifted */
        constexpr ResultImpl(Error<E>&& e)
        // noexcept(is_nothrow_move_constructible_from_unexpected_v<ResultType, E>) :
        noexcept(noexcept(ResultType(std::unexpected(std::declval<E&&>())))) :
            result_(std::unexpected(std::move(e).unwrap())) {}

        [[nodiscard]] constexpr bool has_value() const noexcept {
            return this->result_.has_value();
        }

        [[nodiscard]] constexpr bool has_error() const noexcept {
            return this->result_.has_error();
        }

        // TODO: value
        // TODO: error
        // TODO: and_then
        // TODO: transform
        // TODO: handle
        // TODO: unject

        private:

        ResultType result_;

    };

    // Unpack a Row into a ResultImpl.

    template <typename R, typename U>
    struct result_row_adapter;

    template <typename R, typename... Es>
    struct result_row_adapter<R, Row<Es...>> : std::type_identity<ResultImpl<R, Es...>> {};

    template <typename R, typename U>
    using result_row_adapter_t = typename result_row_adapter<R, U>::type;

} // namespace detail

// TODO: The public-facing concept should probably be named "IsErrorRow" rather
// than "IsRankedPack" since that is an implementation detail. Should also have
// a concept for "IsUniverse" to prevent error cascades when "R" does not provi-
// de a rank function. Unfortunately we can't test injectivity.

template <typename R, typename T, IsTriviallyStorable... Es>
requires detail::IsRankedPack<R, Es...>
using Result = detail::result_row_adapter<R, detail::pack_normalize_t<R, Es...>>;

} // namespace varerr

#endif // VARERR_RESULT_HPP
