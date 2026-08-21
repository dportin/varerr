#ifndef VARERR_RESULT_HPP
#define VARERR_RESULT_HPP

#include "status.hpp"

#include <cassert>
#include <cstddef>

#include <concepts>
#include <expected>
#include <optional>
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

    template <typename R, typename T, IsTriviallyStorable... Es>
    requires IsNormalizedPack<R, Es...>
    struct ResultImpl final {

        using ValueType = T;
        using ErrorType = detail::StatusImpl<R, Es...>;
        using ResultType = std::expected<ValueType, ErrorType>;

        // Construct a ResultImpl from a T.

        explicit constexpr ResultImpl(const T& r)
        noexcept(noexcept(ResultType(r))) :
            result_(r) {}

        explicit constexpr ResultImpl(T&& r)
        noexcept(noexcept(ResultType(r))) :
            result_(std::move(r)) {}

        // Construct a ResultImpl from an Error.

        template <typename E>
        requires row_elem_normalized_v<R, E, Row<Es...>> /* lifted */
        constexpr ResultImpl(const Error<E>& e)
        noexcept(noexcept(ResultType(std::unexpected(std::declval<const E&>())))) :
            result_(std::unexpected(e.unwrap())) {}
        
        template <typename E>
        requires row_elem_normalized_v<R, E, Row<Es...>> /* lifted */
        constexpr ResultImpl(Error<E>&& e)
        noexcept(noexcept(ResultType(std::unexpected(std::declval<E&&>())))) :
            result_(std::unexpected(std::move(e).unwrap())) {}

        // TODO: Consider providing a static factory method to emplace-construct
        // an unexpected/error.

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
            if constexpr (row_elem_normalized_v<R, E, Row<Es...>>) {
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
            static_assert(row_elem_normalized_v<R, E, Row<Es...>>);            
            assert(self.template holds_error<E>());
            return std::forward_like<Self>(*self.status().template get_if<E>());
        }

        template <typename E, typename Self>
        [[nodiscard]] constexpr const_preserving_pointer_t<Self, E> error_if(this Self& self) noexcept {
            static_assert(row_elem_normalized_v<R, E, Row<Es...>>);
            return self.status().template get_if<E>();
        }

        // TODO: and_then
        // TODO: transform
        // TODO: handle
        // TODO: unject

        private:

        [[nodiscard]] const ErrorType& status() const & {
            assert(this->has_error());
            return this->result_.error();
        }

        [[nodiscard]] ErrorType& status() & {
            assert(this->has_error());
            return this->result_.error();
        }

        ResultType result_;

    };

    // Unpack a Row into a ResultImpl.

    template <typename R, typename T, typename U>
    struct result_row_adapter;

    template <typename R, typename T, typename... Es>
    struct result_row_adapter<R, T, Row<Es...>> : std::type_identity<ResultImpl<R, T, Es...>> {};

    template <typename R, typename T, typename U>
    using result_row_adapter_t = typename result_row_adapter<R, T, U>::type;

} // namespace detail

// TODO: The public-facing concept should probably be named "IsErrorRow" rather
// than "IsRankedPack" since that is an implementation detail. Should also have
// a concept for "IsUniverse" to prevent error cascades when "R" does not provi-
// de a rank function. Unfortunately we can't test injectivity.

template <typename R, typename T, IsTriviallyStorable... Es>
requires detail::IsRankedPack<R, Es...>
using Result = detail::result_row_adapter_t<R, T, detail::pack_normalize_t<R, Es...>>;

} // namespace varerr

#endif // VARERR_RESULT_HPP
