#include <varerr/storage.hpp>
#include <varerr/algebra.hpp>

#include <array>
#include <concepts>
#include <cstddef>
#include <utility>

#include "../include/utilities.hpp"
#include "../include/universe.hpp"

// This file instantiates every template and constexpr code path in the headers
// to enable the Clang-Tidy test. Use -ftime-trace to verify coverage.

using namespace varerr::tests;
using namespace varerr::tests::universe;

namespace {

// Instantiate a template.

template <typename T>
constexpr void instantiate(const T&) noexcept {}

// Instantiate the Storage type.

template <typename... Es>
[[maybe_unused]] constexpr void instantiate_storage_type() {

    constexpr std::size_t N = sizeof...(Es);
    using S = varerr::detail::Storage<Es...>;

    // Instantiate the in-place (forwarding) constructors.

    instantiate(S {});
    iterate_index_sequence<N>([]<std::size_t I>(const index_constant<I>) -> void {
        instantiate(S { std::in_place_index<I>, I });
    });

}

template void instantiate_storage_type<>();
template void instantiate_storage_type<E<0>, E<1>, E<2>>();

// Instantiate the Storage traits.

template <typename... Es>
[[maybe_unused]] constexpr void instantiate_storage_traits() {

    constexpr std::size_t N = sizeof...(Es);
    using S = varerr::detail::Storage<Es...>;

    static_assert(varerr::detail::IsStorage<S>);
    static_assert(varerr::detail::storage_size_v<S> == sizeof...(Es));

    iterate_index_sequence<N>([]<std::size_t I>(const index_constant<I>) -> void {
        static_assert(std::same_as<
            varerr::detail::storage_alternative_t<I, S>,
            varerr::detail::pack_subscript_t<I, Es...>
        >);
    });

}

template void instantiate_storage_traits<>();
template void instantiate_storage_traits<E<0>, E<1>, E<2>>();

// Instantiate the Storage interface.

template <typename... Es>
[[maybe_unused]] constexpr void instantiate_storage_interface() {

    constexpr std::size_t N = sizeof...(Es);
    using S = varerr::detail::Storage<Es...>;

    // Build array of Storage objects with alternative I active at index I.

    std::array<S, N> stores {};
    iterate_index_sequence<N>([&]<std::size_t I>(const index_constant<I>) -> void {
        stores[I] = S { std::in_place_index<I>, I };
    });

    // Instantiate storage_get.

    iterate_index_sequence<N>([&]<std::size_t I>(const index_constant<I>) -> void {
        instantiate(varerr::detail::storage_get<I>(stores[I]));
    });

    // Instantiate storage_emplace.

    iterate_index_sequence<N>([&]<std::size_t I>(const index_constant<I>) -> void {
        instantiate(varerr::detail::storage_emplace<I>(stores[I], I));
    });

}

template void instantiate_storage_interface<>();
template void instantiate_storage_interface<E<0>, E<1>, E<2>>();

// Instantiate the row algebra traits.

template <typename M, typename U>
[[maybe_unused]] constexpr void instantiate_algebra_traits_unary() {

    constexpr std::size_t N = varerr::row_size_v<U>;

    static_assert(varerr::IsNormalizedRow<M, U>);
    static_assert(std::same_as<varerr::row_normalize_t<M, pack_reverse_t<U>>, U>);

    iterate_index_sequence<N>([&]<std::size_t I>(const index_constant<I>) -> void {
        static_assert(varerr::row_lookup_normalized_v<M, E<I>, U>.value_or(N) == I);
    });

}

template void instantiate_algebra_traits_unary<UniverseE, varerr::Row<>>();
template void instantiate_algebra_traits_unary<UniverseE, varerr::Row<E<0>, E<1>, E<2>>>();

// Instantiate the row algebra operations.

template <typename M, typename U, typename V>
[[maybe_unused]] constexpr void instantiate_algebra_traits_binary() {

    instantiate(varerr::row_size_v<U>);
    instantiate(varerr::row_subset_normalized_v<M, U, V>);
    instantiate(varerr::row_equiv_normalized_v<M, U, V>);
    instantiate(varerr::is_normalized_row_v<M, varerr::row_union_normalized_t<M, U, V>>);
    instantiate(varerr::is_normalized_row_v<M, varerr::row_intersection_normalized_t<M, U, V>>);
    instantiate(varerr::is_normalized_row_v<M, varerr::row_difference_normalized_t<M, V, U>>);

}

template void instantiate_algebra_traits_binary<UniverseE, varerr::Row<>, varerr::Row<>>();
template void instantiate_algebra_traits_binary<UniverseE, varerr::Row<E<0>, E<2>>, varerr::Row<E<0>, E<1>, E<2>, E<3>>>();

} // namespace
