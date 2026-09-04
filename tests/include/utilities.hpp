#ifndef VARERR_TESTS_UTILITIES
#define VARERR_TESTS_UTILITIES

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <limits>
#include <type_traits>
#include <utility>

namespace varerr::tests {

// Invoke a function F for every index I in [N].

template <std::size_t N, typename F>
constexpr void iterate_index_sequence(F f) {
    [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        (f(std::integral_constant<std::size_t, Is> {}), ...);
    }(std::make_index_sequence<N> {});
}

// Invoke a function F for every index I in Is.

template <std::size_t... Is, typename F>
constexpr void iterate_index_array(F f) {
    (f(std::integral_constant<std::size_t, Is> {}), ...);
}

// Invoke a function F for every cvref-qualified version of T.

template <typename T, typename F>
constexpr void iterate_cvref_matrix(F f) {
    f(std::type_identity<T> {});
    f(std::type_identity<const T> {});
    f(std::type_identity<volatile T> {});
    f(std::type_identity<const volatile T> {});
    f(std::type_identity<T&> {});
    f(std::type_identity<const T&> {});
    f(std::type_identity<volatile T&> {});
    f(std::type_identity<const volatile T&> {});
    f(std::type_identity<T&&> {});
    f(std::type_identity<const T&&> {});
    f(std::type_identity<volatile T&&> {});
    f(std::type_identity<const volatile T&&> {});
}

// Apply the qualifiers and value category of S to E.

template <typename S, typename E>
using cv_qualify_like_t = std::conditional_t<
    std::is_const_v<std::remove_reference_t<S>>,
    std::conditional_t<std::is_volatile_v<std::remove_reference_t<S>>, const volatile E, const E>,
    std::conditional_t<std::is_volatile_v<std::remove_reference_t<S>>, volatile E, E>
>;

template <typename S, typename E>
using cvref_qualify_like_t = std::conditional_t<
    std::is_lvalue_reference_v<S>,
    cv_qualify_like_t<S, E>&,
    cv_qualify_like_t<S, E>&&
>;

// Lift an index sequence to an indexed type E and store the result in M.

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

// Apply a metafunction F to the type parameters of M.

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

// Reverse the elements of a parameter pack.

namespace detail {

template <template <typename...> typename M, typename Fs, typename... Es>
struct pack_reverse_impl_rec : std::type_identity<Fs> {};

template <template <typename...> typename M, typename... Fs, typename E, typename... Es>
struct pack_reverse_impl_rec<M, M<Fs...>, E, Es...> : pack_reverse_impl_rec<M, M<E, Fs...>, Es...> {};

template <typename M>
struct pack_reverse_impl;

template <template <typename...> typename M, typename... Es>
struct pack_reverse_impl<M<Es...>> : pack_reverse_impl_rec<M, M<>, Es...> {};

} // namespace detail

template <typename M>
using pack_reverse_t = detail::pack_reverse_impl<M>::type;

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

// Round an unsigned integer up to the nearest multiple.

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

// Compute the integer square root of a std::size_t using binary search.

[[nodiscard]] constexpr std::size_t isqrt(std::size_t z) noexcept {

    if (z < 2) {
        return z;
    }

    std::size_t lo = 1;
    std::size_t hi = z / 2;

    while (lo <= hi) {
        const auto mid = lo + (hi - lo) / 2;
        if (mid <= z / mid) {
            lo = mid + 1;
        } else {
            hi = mid - 1;
        }
    }

    return hi;

}

} // namespace varerr::tests

#endif // VARERR_TESTS_UTILITIES
