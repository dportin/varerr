#ifndef VARERR_TESTS_UTILITIES
#define VARERR_TESTS_UTILITIES

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <limits>
#include <type_traits>
#include <utility>

namespace varerr::tests {

template <std::size_t N, typename F>
constexpr void iterate_index_sequence(F f) {
    [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        (f(std::integral_constant<std::size_t, Is>{}), ...);
    }(std::make_index_sequence<N>{});
}

namespace detail {

template <template <typename...> typename M, template <std::size_t> typename E, typename Is>
struct lift_index_sequence_impl;

template <template <typename...> typename M, template <std::size_t> typename E, std::size_t... Is>
struct lift_index_sequence_impl<M, E, std::index_sequence<Is...>> : std::type_identity<M<E<Is>...>> {};

} // namespace detail

template <template <typename...> typename M, template <std::size_t> typename E, std::size_t N>
struct lift_index_sequence : detail::lift_index_sequence_impl<M, E, std::make_index_sequence<N>> {};

template <template <typename...> typename M, template <std::size_t> typename E, std::size_t N>
using lift_index_sequence_t = lift_index_sequence<M, E, N>::type;

namespace detail {

template <typename F, typename M>
struct pack_apply;

template <typename F, template <typename...> typename M, typename... Es>
requires requires { typename F::template apply<Es...>; }
struct pack_apply<F, M<Es...>> : std::type_identity<typename F::template apply<Es...>> {};

} // namespace detail

template <typename F, typename M>
using pack_apply_t = detail::pack_apply<F, M>::type;

template <typename F, typename M>
constexpr auto pack_apply_v = pack_apply_t<F, M>::value;

// Determine the maximum size of the elements of a parameter pack.

template <typename... Es>
requires (sizeof...(Es) > 0)
constexpr std::size_t max_sizeof_v = std::max({sizeof(Es)...});

struct max_sizeof_fun {
    template <typename... Es>
    using apply = std::integral_constant<std::size_t, max_sizeof_v<Es...>>;
};

// Determine the maximum alignment of the elements of a parameter pack.

template <typename... Es>
requires (sizeof...(Es) > 0)
constexpr std::size_t max_alignof_v = std::max({alignof(Es)...});

struct max_alignof_fun {
    template <typename... Es>
    using apply = std::integral_constant<std::size_t, max_alignof_v<Es...>>;
};

// Round an unsigned integer to the nearest multiple.

template <std::unsigned_integral T>
[[nodiscard]] constexpr T round_to_multiple(T value, T multiple) noexcept {

    assert(multiple > 0);
    const T remainder = value % multiple;

    if (remainder) {
        assert(value <= std::numeric_limits<T>::max() - (multiple - remainder));
        return static_cast<T>(value + (multiple - remainder));
    } else {
        return value;
    }

}

} // namespace varerr::tests

#endif // VARERR_TESTS_UTILITIES
