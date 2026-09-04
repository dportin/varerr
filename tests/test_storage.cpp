#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_template_test_macros.hpp>

#include "include/utilities.hpp"
#include "include/universe.hpp"

#include <varerr/storage.hpp>

#include <concepts>
#include <cstddef>
#include <type_traits>
#include <utility>

using namespace varerr::tests;
using namespace varerr::tests::universe;

namespace {

// Lift the homogeneous test universe to Storage.

template <std::size_t N>
using S = lift_index_sequence_t<varerr::detail::Storage, E, N>;

static_assert(std::same_as<S<0>, varerr::detail::Storage<>>);
static_assert(std::same_as<S<1>, varerr::detail::Storage<E<0>>>);
static_assert(std::same_as<S<2>, varerr::detail::Storage<E<0>, E<1>>>);

// Lift the heterogeneous test universe to Storage for a fixed log-alignment.

template <std::size_t A>
struct fixed_align_bind_adapter {
    template <std::size_t N>
    using type = H<N, A>;
};

template <std::size_t N, std::size_t A>
using L = lift_index_sequence_t<varerr::detail::Storage, fixed_align_bind_adapter<A>::template type, N>;

static_assert(std::same_as<L<0,3>, varerr::detail::Storage<>>);
static_assert(std::same_as<L<1,3>, varerr::detail::Storage<H<0,3>>>);
static_assert(std::same_as<L<2,3>, varerr::detail::Storage<H<0,3>, H<1,3>>>);

// Round the size of the storage alternatives up to their maximum alignment.

template <typename... Es>
constexpr std::size_t storage_sizeof_v = round_to_multiple(max_sizeof_v<Es...>, max_alignof_v<Es...>);

struct storage_sizeof_fun {
    template <typename... Es>
    using apply = std::integral_constant<std::size_t, storage_sizeof_v<Es...>>;
};

// Determine whether a storage_get call is well-formed.

template <typename S, std::size_t N>
concept IsStorageGetWellFormed = requires {
    varerr::detail::storage_get<N>(std::declval<S>());
};


// Determine whether a storage_emplace call is well-formed.

template <typename S, std::size_t N, typename... Args>
concept IsStorageEmplaceWellFormed = requires {
    varerr::detail::storage_emplace<N>(std::declval<S>(), std::declval<Args>()...);
};

} // namespace

TEMPLATE_TEST_CASE("varerr_storage_trivial", "[varerr][storage]",
    S<0>, S<1>, S<2>, S<3>,
    (varerr::detail::Storage<E<0>, H<4,3>, H<0,13>>)
) {

    STATIC_REQUIRE(std::is_trivially_copyable_v<TestType>);
    STATIC_REQUIRE(std::is_trivially_destructible_v<TestType>);
    STATIC_REQUIRE(std::is_trivially_copy_constructible_v<TestType>);
    STATIC_REQUIRE(std::is_trivially_move_constructible_v<TestType>);
    STATIC_REQUIRE(std::is_trivially_copy_assignable_v<TestType>);
    STATIC_REQUIRE(std::is_trivially_move_assignable_v<TestType>);

    // The default constructor is explicitly defined and thus non-trivial.

    STATIC_REQUIRE(std::is_default_constructible_v<TestType>);
    STATIC_REQUIRE_FALSE(std::is_trivially_default_constructible_v<TestType>);

    // The storage class propagates but does not enforce the properties of its
    // alternatives. The alternatives are further contrained in BasicStatus.

    STATIC_REQUIRE(std::is_trivially_copyable_v<varerr::detail::Storage<NoCopyConstructType>>);
    STATIC_REQUIRE_FALSE(std::is_copy_constructible_v<varerr::detail::Storage<NoCopyConstructType>>);

    STATIC_REQUIRE(std::is_trivially_copyable_v<varerr::detail::Storage<NoCopyAssignType>>);
    STATIC_REQUIRE_FALSE(std::is_copy_assignable_v<varerr::detail::Storage<NoCopyAssignType>>);

    STATIC_REQUIRE(std::is_trivially_copyable_v<varerr::detail::Storage<NoMoveConstructType>>);
    STATIC_REQUIRE_FALSE(std::is_move_constructible_v<varerr::detail::Storage<NoMoveConstructType>>);

    STATIC_REQUIRE(std::is_trivially_copyable_v<varerr::detail::Storage<NoMoveAssignType>>);
    STATIC_REQUIRE_FALSE(std::is_move_assignable_v<varerr::detail::Storage<NoMoveAssignType>>);

}

TEMPLATE_TEST_CASE("varerr_storage_noexcept", "[varerr][storage]",
    S<0>, S<1>, S<2>, S<3>,
    (varerr::detail::Storage<E<0>, H<4,3>, H<0,13>>)
) {

    STATIC_REQUIRE(std::is_nothrow_default_constructible_v<TestType>);
    STATIC_REQUIRE(std::is_nothrow_copy_constructible_v<TestType>);
    STATIC_REQUIRE(std::is_nothrow_move_constructible_v<TestType>);
    STATIC_REQUIRE(std::is_nothrow_copy_assignable_v<TestType>);
    STATIC_REQUIRE(std::is_nothrow_move_assignable_v<TestType>);
    STATIC_REQUIRE(std::is_nothrow_destructible_v<TestType>);

}

TEST_CASE("varerr_storage_noexcept_conditional_default", "[varerr][storage]") {

    // The default constructor is unconditionally noexcept.

    STATIC_REQUIRE_FALSE(std::is_default_constructible_v<ConditionalThrowType>);
    STATIC_REQUIRE(std::is_nothrow_default_constructible_v<varerr::detail::Storage<ConditionalThrowType>>);
    STATIC_REQUIRE(std::is_nothrow_default_constructible_v<varerr::detail::Storage<NoDefaultConstructType>>);

}

TEST_CASE("varerr_storage_noexcept_conditional_construct", "[varerr][storage]") {

    using S0 = varerr::detail::Storage<E<0>, ConditionalThrowType, E<2>>;

    // The in-place constructor is conditionally noexcept.

    STATIC_REQUIRE(noexcept(S0(std::in_place_index<0>, std::declval<std::size_t>())));
    STATIC_REQUIRE(noexcept(S0(std::in_place_index<2>, std::declval<std::size_t>())));

    STATIC_REQUIRE(noexcept(S0(std::in_place_index<1>, std::declval<int>())));
    STATIC_REQUIRE_FALSE(noexcept(S0(std::in_place_index<1>, std::declval<double>())));

}

TEST_CASE("varerr_storage_noexcept_conditional_emplace", "[varerr][storage]") {

    using S0 = varerr::detail::Storage<E<0>, ConditionalThrowType, E<2>>;

    // The emplace constructor is conditionally noexcept.

    STATIC_REQUIRE(noexcept(varerr::detail::storage_emplace<0>(std::declval<S0&>(), std::declval<std::size_t>())));
    STATIC_REQUIRE(noexcept(varerr::detail::storage_emplace<2>(std::declval<S0&>(), std::declval<std::size_t>())));

    STATIC_REQUIRE(noexcept(varerr::detail::storage_emplace<1>(std::declval<S0&>(), std::declval<int>())));
    STATIC_REQUIRE_FALSE(noexcept(varerr::detail::storage_emplace<1>(std::declval<S0&>(), std::declval<double>())));

}

TEST_CASE("varerr_storage_noexcept_conditional_get", "[varerr][storage]") {

    using S0 = varerr::detail::Storage<E<0>, ConditionalThrowType, E<2>>;

    // The access operation is unconditionally noexcept.

    iterate_cvref_matrix<S0>([]<typename T>(const std::type_identity<T>) {
        STATIC_REQUIRE(noexcept(varerr::detail::storage_get<0>(std::declval<T>())));
        STATIC_REQUIRE(noexcept(varerr::detail::storage_get<1>(std::declval<T>())));
        STATIC_REQUIRE(noexcept(varerr::detail::storage_get<2>(std::declval<T>())));
    });

}

TEMPLATE_TEST_CASE("varerr_storage_memory", "[varerr][storage]",
    S<1>, S<2>, S<3>, S<4>,
    (varerr::detail::Storage<H<1,13>, H<64,0>, H<8,8>>),
    (varerr::detail::Storage<H<64,0>, H<1,13>, H<8,8>>),
    (varerr::detail::Storage<H<64,0>, H<8,8>, H<1,13>>),
    (varerr::detail::Storage<H<0,13>, H<4,2>>),
    (varerr::detail::Storage<H<4,2>, H<0,13>>)
) {

    // The storage size is the size of the widest alternative rounded up to the
    // alignment of the strictest alternative.

    STATIC_REQUIRE(sizeof(TestType) == pack_apply_v<storage_sizeof_fun, TestType>);
    STATIC_REQUIRE(alignof(TestType) == pack_apply_v<max_alignof_fun, TestType>);

    // The storage size must be greater than the size of any alternative and be
    // a valid array element size.

    STATIC_REQUIRE(sizeof(TestType) >= pack_apply_v<max_sizeof_fun, TestType>);
    STATIC_REQUIRE(sizeof(TestType) % alignof(TestType) == 0);

    // The storage must not add a discriminant or padding beyond what is requi-
    // red by its alignment.

    STATIC_REQUIRE(sizeof(TestType) < pack_apply_v<max_sizeof_fun, TestType> + alignof(TestType));

    // Permuting the alternatives does not affect the storage size.

    STATIC_REQUIRE(sizeof(TestType) == sizeof(pack_reverse_t<TestType>));
    STATIC_REQUIRE(alignof(TestType) == alignof(pack_reverse_t<TestType>));

}

TEST_CASE("varerr_storage_memory_empty", "[varerr][storage]") {

    // The size and alignment of the empty storage is one byte on every target
    // implementation. However, the standard only requires them to be positive.

    STATIC_REQUIRE(sizeof(S<0>) == 1);
    STATIC_REQUIRE(alignof(S<0>) == 1);

}

TEMPLATE_TEST_CASE("varerr_storage_layout", "[varerr][storage]",
    S<0>, S<1>, S<2>, S<3>,
    (varerr::detail::Storage<H<0,1>>),
    (varerr::detail::Storage<H<2,2>, H<3,3>, H<4,4>>),
    (varerr::detail::Storage<H<7,3>, H<15,4>, H<13,3>>)
) {

    // A union is never an aggregate if it declares a constructor.

    STATIC_REQUIRE(std::is_standard_layout_v<TestType>);
    STATIC_REQUIRE_FALSE(std::is_aggregate_v<TestType>);

}

TEMPLATE_TEST_CASE("varerr_storage_offset", "[varerr][storage]",
    S<3>,
    (varerr::detail::Storage<H<0,5>, H<4,0>, H<8,5>>)
) {

    // Every alternative begins at the address of the storage itself.

    TestType storage {};
    const auto* base_addr = std::addressof(storage);

    iterate_index_sequence<varerr::detail::storage_size_v<TestType>>([&](const auto index) -> void {

        // Instantiate the S and H alternatives with the index I.

        constexpr std::size_t I = decltype(index)::value;
        using E = varerr::detail::storage_alternative_t<I, TestType>;
        const auto* elem_addr = varerr::detail::storage_emplace<I>(storage, E(I));

        CAPTURE(I);
        REQUIRE(static_cast<const void*>(base_addr) == static_cast<const void*>(elem_addr));
        REQUIRE(reinterpret_cast<std::uintptr_t>(elem_addr) % alignof(E) == 0);

    });

}

TEMPLATE_TEST_CASE("varerr_storage_offset_member", "[varerr][storage]",
    S<3>,
    (varerr::detail::Storage<H<0,5>, H<4,0>, H<8,5>>)
) {

    STATIC_REQUIRE(offsetof(TestType, head_) == 0);
    STATIC_REQUIRE(offsetof(TestType, tail_) == 0);
    STATIC_REQUIRE(offsetof(TestType, tail_.head_) == 0);
    STATIC_REQUIRE(offsetof(TestType, tail_.tail_) == 0);
    STATIC_REQUIRE(offsetof(TestType, tail_.tail_.head_) == 0);
    STATIC_REQUIRE(offsetof(TestType, tail_.tail_.tail_) == 0);

}

TEST_CASE("varerr_storage_traits_is_storable", "[varerr][storage]") {

    STATIC_REQUIRE(varerr::IsStorable<E<0>>);
    STATIC_REQUIRE(varerr::IsStorable<E<1>>);
    STATIC_REQUIRE(varerr::IsStorable<H<0,3>>);
    STATIC_REQUIRE(varerr::IsStorable<H<8,3>>);

    STATIC_REQUIRE(varerr::IsStorable<S<0>>);
    STATIC_REQUIRE(varerr::IsStorable<S<10>>);
    STATIC_REQUIRE(varerr::IsStorable<L<0,3>>);
    STATIC_REQUIRE(varerr::IsStorable<L<10,3>>);

    STATIC_REQUIRE(varerr::IsStorable<int*>);
    STATIC_REQUIRE(varerr::IsStorable<bool(*)(int)>);
    STATIC_REQUIRE(varerr::IsStorable<std::nullptr_t>);

    STATIC_REQUIRE_FALSE(varerr::IsStorable<void>);
    STATIC_REQUIRE_FALSE(varerr::IsStorable<bool(int)>);
    STATIC_REQUIRE_FALSE(varerr::IsStorable<int[3]>);
    STATIC_REQUIRE_FALSE(varerr::IsStorable<int[]>);

    STATIC_REQUIRE(varerr::IsStorable<NoDefaultConstructType>);
    STATIC_REQUIRE(varerr::IsStorable<NoCopyConstructType>);
    STATIC_REQUIRE(varerr::IsStorable<NoCopyAssignType>);
    STATIC_REQUIRE(varerr::IsStorable<NoMoveConstructType>);
    STATIC_REQUIRE(varerr::IsStorable<NoMoveAssignType>);
    STATIC_REQUIRE(varerr::IsStorable<NonTrivialConstructType>);
    STATIC_REQUIRE(varerr::IsStorable<NonTrivialDestructType>);
    STATIC_REQUIRE(varerr::IsStorable<NonTrivialCopyConstructType>);
    STATIC_REQUIRE(varerr::IsStorable<NonTrivialCopyAssignType>);
    STATIC_REQUIRE(varerr::IsStorable<NonTrivialMoveConstructType>);
    STATIC_REQUIRE(varerr::IsStorable<NonTrivialMoveAssignType>);
    STATIC_REQUIRE(varerr::IsStorable<NonStandardLayoutType>);
    STATIC_REQUIRE(varerr::IsStorable<ConditionalThrowType>);

    // No qualification of a storable T is storable.

    iterate_cvref_matrix<int>([]<typename T>(const std::type_identity<T>) {
        if constexpr (std::is_same_v<T, std::remove_cvref_t<T>>) {
            STATIC_REQUIRE(varerr::IsStorable<T>);
        } else {
            STATIC_REQUIRE_FALSE(varerr::IsStorable<T>);
        }
    });

}

TEST_CASE("varerr_storage_traits_is_trivially_storable", "[varerr][storage]") {

    STATIC_REQUIRE(varerr::IsTriviallyStorable<E<0>>);
    STATIC_REQUIRE(varerr::IsTriviallyStorable<E<1>>);
    STATIC_REQUIRE(varerr::IsTriviallyStorable<H<0,3>>);
    STATIC_REQUIRE(varerr::IsTriviallyStorable<H<8,3>>);

    STATIC_REQUIRE(varerr::IsTriviallyStorable<S<0>>);
    STATIC_REQUIRE(varerr::IsTriviallyStorable<S<10>>);
    STATIC_REQUIRE(varerr::IsTriviallyStorable<L<0,3>>);
    STATIC_REQUIRE(varerr::IsTriviallyStorable<L<10,3>>);

    STATIC_REQUIRE(varerr::IsTriviallyStorable<int*>);
    STATIC_REQUIRE(varerr::IsTriviallyStorable<bool(*)(int)>);
    STATIC_REQUIRE(varerr::IsTriviallyStorable<std::nullptr_t>);

    STATIC_REQUIRE_FALSE(varerr::IsTriviallyStorable<void>);
    STATIC_REQUIRE_FALSE(varerr::IsTriviallyStorable<bool(int)>);
    STATIC_REQUIRE_FALSE(varerr::IsTriviallyStorable<int[3]>);
    STATIC_REQUIRE_FALSE(varerr::IsTriviallyStorable<int[]>);

    STATIC_REQUIRE(varerr::IsTriviallyStorable<NoDefaultConstructType>);
    STATIC_REQUIRE(varerr::IsTriviallyStorable<NoCopyConstructType>);
    STATIC_REQUIRE(varerr::IsTriviallyStorable<NoCopyAssignType>);
    STATIC_REQUIRE(varerr::IsTriviallyStorable<NoMoveConstructType>);
    STATIC_REQUIRE(varerr::IsTriviallyStorable<NoMoveAssignType>);
    STATIC_REQUIRE(varerr::IsTriviallyStorable<NonStandardLayoutType>);
    STATIC_REQUIRE(varerr::IsTriviallyStorable<ConditionalThrowType>);

    // A trivially storable type may have a non-trivial constructor.

    STATIC_REQUIRE(varerr::IsTriviallyStorable<NonTrivialConstructType>);

    // A trivially storable type must have a trivial destructor and trivial copy
    // and move constructors and assignment operations.

    STATIC_REQUIRE_FALSE(varerr::IsTriviallyStorable<NonTrivialDestructType>);
    STATIC_REQUIRE_FALSE(varerr::IsTriviallyStorable<NonTrivialCopyConstructType>);
    STATIC_REQUIRE_FALSE(varerr::IsTriviallyStorable<NonTrivialCopyAssignType>);
    STATIC_REQUIRE_FALSE(varerr::IsTriviallyStorable<NonTrivialMoveConstructType>);
    STATIC_REQUIRE_FALSE(varerr::IsTriviallyStorable<NonTrivialMoveAssignType>);

    // No qualification of a trivially storable T is trivially storable.

    iterate_cvref_matrix<int>([]<typename T>(const std::type_identity<T>) {
        if constexpr (std::is_same_v<T, std::remove_cvref_t<T>>) {
            STATIC_REQUIRE(varerr::IsTriviallyStorable<T>);
        } else {
            STATIC_REQUIRE_FALSE(varerr::IsTriviallyStorable<T>);
        }
    });

}

TEST_CASE("varerr_storage_traits_is_storage", "[varerr][storage]") {

    STATIC_REQUIRE(varerr::detail::IsStorage<S<0>>);
    STATIC_REQUIRE(varerr::detail::IsStorage<S<1>>);

    iterate_cvref_matrix<S<1>>([]<typename T>(const std::type_identity<T>) {
        STATIC_REQUIRE(varerr::detail::is_storage_v<T>);
        STATIC_REQUIRE(varerr::detail::IsStorage<T>);
    });

}

TEST_CASE("varerr_storage_traits_storage_size", "[varerr][storage]") {

    constexpr std::size_t kStorageSizeTestBound = 10;

    iterate_index_sequence<kStorageSizeTestBound>([&](const auto index) {
        constexpr std::size_t I = decltype(index)::value;
        iterate_cvref_matrix<S<I>>([]<typename T>(const std::type_identity<T>) {
            STATIC_REQUIRE(varerr::detail::storage_size_v<T> == I);
        });
    });

}

TEST_CASE("varerr_storage_traits_storage_alternative", "[varerr][storage]") {

    constexpr std::size_t kStorageAlternativeTestBound = 10;

    iterate_index_sequence<kStorageAlternativeTestBound>([&](const auto index) {
        constexpr std::size_t I = decltype(index)::value;
        iterate_cvref_matrix<S<kStorageAlternativeTestBound>>([]<typename T>(const std::type_identity<T>) {
            STATIC_REQUIRE(std::same_as<varerr::detail::storage_alternative_t<I, T>, E<I>>);
        });
    });

}

TEST_CASE("varerr_storage_interface_storage_get_forward", "[varerr][storage]") {

    constexpr std::size_t kStorageGetForwardTestAlignBound = 3;
    constexpr std::size_t kStorageGetForwardTestSizeBound = 10;

    using L0 = L<kStorageGetForwardTestSizeBound, kStorageGetForwardTestAlignBound>;

    iterate_index_sequence<kStorageGetForwardTestSizeBound>([&](const auto index) {
        constexpr std::size_t I = decltype(index)::value;
        iterate_cvref_matrix<L0>([]<typename S>(const std::type_identity<S>) {
            STATIC_REQUIRE(std::same_as<
                decltype(varerr::detail::storage_get<I>(std::declval<S>())),
                cvref_qualify_like_t<S, H<I, kStorageGetForwardTestAlignBound>>
            >);
        });
    });

}

TEST_CASE("varerr_storage_interface_storage_get_noexcept", "[varerr][storage]") {

    constexpr std::size_t kStorageGetNoExceptTestAlignBound = 3;
    constexpr std::size_t kStorageGetNoExceptTestSizeBound = 10;

    using L0 = L<kStorageGetNoExceptTestSizeBound, kStorageGetNoExceptTestAlignBound>;

    iterate_index_sequence<kStorageGetNoExceptTestSizeBound>([&](const auto index) {
        constexpr std::size_t I = decltype(index)::value;
        iterate_cvref_matrix<L0>([]<typename S>(const std::type_identity<S>) {
            STATIC_REQUIRE(noexcept(varerr::detail::storage_get<I>(std::declval<S>())));
        });
    });

}

TEST_CASE("varerr_storage_interface_storage_emplace_noexcept", "[varerr][storage]") {

    using L1 = varerr::detail::Storage<E<0>, ConditionalThrowType, E<1>>;

    STATIC_REQUIRE(noexcept(varerr::detail::storage_emplace<1>(std::declval<L1&>(), std::declval<int>())));
    STATIC_REQUIRE_FALSE(noexcept(varerr::detail::storage_emplace<1>(std::declval<L1&>(), std::declval<double>())));

}

TEST_CASE("varerr_storage_interface_storage_activate", "[varerr][storage]") {

    constexpr std::size_t kStorageEmplaceActivateTestAlignBound = 3;
    constexpr std::size_t kStorageEmplaceActivateTestSizeBound = 10;

    using L0 = L<kStorageEmplaceActivateTestSizeBound, kStorageEmplaceActivateTestAlignBound>;

    // Activate each alternative twice using linear permutations of [N]. The
    // strides must be coprime with the number of alternatives.

    L0 storage {};

    iterate_index_array<7, 13>([&](const auto stride) { /* 7, 13 _|_ 10 */

        constexpr std::size_t K = decltype(stride)::value;
        iterate_index_sequence<kStorageEmplaceActivateTestSizeBound>([&](const auto index) {

            constexpr std::size_t I = decltype(index)::value;
            constexpr std::size_t J = (I * K) % kStorageEmplaceActivateTestSizeBound;
            using A = varerr::detail::storage_alternative_t<J, L0>;

            varerr::detail::storage_emplace<J>(storage, A(I));
            const auto bytes = varerr::detail::storage_get<J>(storage).bytes();

            CAPTURE(K); CAPTURE(I); CAPTURE(J);
            REQUIRE(bytes.size() == J);

            for (std::size_t i = 0; i < J; ++i) {
                REQUIRE(bytes[i] == static_cast<unsigned char>(I >> (CHAR_BIT * (i % sizeof(std::size_t)))));
            }

        });

    });

}

TEST_CASE("varerr_storage_functional_replace_head", "[varerr][storage]") {

        STATIC_REQUIRE([]{
            S<2> storage(std::in_place_index<0>, std::size_t {100});
            return storage.head_;
        }().value() == 100);

        STATIC_REQUIRE([]{
            S<2> storage(std::in_place_index<1>, std::size_t {100});
            std::construct_at(std::addressof(storage.head_), std::size_t {101});
            return storage.head_;
        }().value() == 101);

}

TEST_CASE("varerr_storage_functional_replace_tail", "[varerr][storage]") {

    STATIC_REQUIRE([]{
        S<2> storage(std::in_place_index<1>, std::size_t {100});
        return storage.tail_.head_;
    }().value() == 100);

    STATIC_REQUIRE([]{
        S<2> storage(std::in_place_index<0>, std::size_t {100});
        std::construct_at(std::addressof(storage.tail_));
        std::construct_at(std::addressof(storage.tail_.head_), std::size_t {101});
        return storage.tail_.head_;
    }().value() == 101);

}

TEST_CASE("varerr_storage_constexpr_construct", "[varerr][storage]") {

    constexpr std::size_t kStorageConstConstructTestBound = 10;
    using S0 = S<kStorageConstConstructTestBound>;

    iterate_index_sequence<kStorageConstConstructTestBound>([&](const auto index) {
        constexpr std::size_t I = decltype(index)::value;
        STATIC_REQUIRE([]{
            S0 storage(std::in_place_index<I>, std::size_t {I + 100});
            return varerr::detail::storage_get<I>(storage).value();
        }() == I + 100);
    });

}

TEST_CASE("varerr_storage_constexpr_emplace", "[varerr][storage]") {

    constexpr std::size_t kStorageConstEmplaceTestBound = 10;
    using S0 = S<kStorageConstEmplaceTestBound>;

    iterate_index_sequence<kStorageConstEmplaceTestBound>([&](const auto index) {
        constexpr std::size_t I = decltype(index)::value;
        STATIC_REQUIRE([]{
            S0 storage {};
            varerr::detail::storage_emplace<I>(storage, E<I>(I + 100));
            return varerr::detail::storage_get<I>(storage).value();
        }() == I + 100);
    });

}

TEST_CASE("varerr_storage_constexpr_access_copy", "[varerr][storage]") {

    constexpr std::size_t kStorageConstAccessCopyTestBound = 10;
    using S0 = S<kStorageConstAccessCopyTestBound>;

    iterate_index_sequence<kStorageConstAccessCopyTestBound>([&](const auto index) {
        constexpr std::size_t I = decltype(index)::value;
        STATIC_REQUIRE([]{
            const S0 storage(std::in_place_index<I>, std::size_t {I + 100});
            return varerr::detail::storage_get<I>(storage).value();
        }() == I + 100);
    });

}

TEST_CASE("varerr_storage_constexpr_access_move", "[varerr][storage]") {

    constexpr std::size_t kStorageConstAccessMoveTestBound = 10;
    using S0 = S<kStorageConstAccessMoveTestBound>;

    iterate_index_sequence<kStorageConstAccessMoveTestBound>([&](const auto index) {
        constexpr std::size_t I = decltype(index)::value;
        STATIC_REQUIRE([]{
            S0 storage(std::in_place_index<I>, std::size_t {I + 100});
            return varerr::detail::storage_get<I>(std::move(storage)).value();
        }() == I + 100);
    });

}

TEST_CASE("varerr_storage_functional_forward_emplace", "[varerr][storage]") {

    using F0 = varerr::detail::Storage<E<0>, ForwardProbeType, E<1>>;

    STATIC_REQUIRE([]{
        F0 storage {}; int fst = 0;
        varerr::detail::storage_emplace<1>(storage, fst);
        return varerr::detail::storage_get<1>(storage).fst_;
    }() == ForwardCategory::LValue);

    STATIC_REQUIRE([]{
        F0 storage {}; const int fst = 0;
        varerr::detail::storage_emplace<1>(storage, fst);
        return varerr::detail::storage_get<1>(storage).fst_;
    }() == ForwardCategory::ConstLValue);

    STATIC_REQUIRE([]{
        F0 storage {};
        varerr::detail::storage_emplace<1>(storage, 0);
        return varerr::detail::storage_get<1>(storage).fst_;
    }() == ForwardCategory::RValue);

    STATIC_REQUIRE([]{
        F0 storage {}; const int fst = 0;
        varerr::detail::storage_emplace<1>(storage, std::move(fst)); // NOLINT
        return varerr::detail::storage_get<1>(storage).fst_;
    }() == ForwardCategory::ConstRValue);

    STATIC_REQUIRE([] -> std::pair<ForwardCategory, ForwardCategory> {
        F0 storage {}; int fst = 0; const int snd = 1;
        varerr::detail::storage_emplace<1>(storage, fst, std::move(snd)); // NOLINT
        return { varerr::detail::storage_get<1>(storage).fst_, varerr::detail::storage_get<1>(storage).snd_ };
    }() == std::pair { ForwardCategory::LValue, ForwardCategory::ConstRValue });

    STATIC_REQUIRE([] -> std::pair<ForwardCategory, ForwardCategory> {
        F0 storage {}; const int fst = 0;
        varerr::detail::storage_emplace<1>(storage, fst, 0);
        return { varerr::detail::storage_get<1>(storage).fst_, varerr::detail::storage_get<1>(storage).snd_ };
    }() == std::pair { ForwardCategory::ConstLValue, ForwardCategory::RValue });

}

TEST_CASE("varerr_storage_functional_forward_construct", "[varerr][storage]") {

    using F0 = varerr::detail::Storage<E<0>, ForwardProbeType, E<1>>;

    STATIC_REQUIRE([]{
        int fst = 0;
        F0 storage(std::in_place_index<1>, fst);
        return varerr::detail::storage_get<1>(storage).fst_;
    }() == ForwardCategory::LValue);

    STATIC_REQUIRE([]{
        const int fst = 0;
        F0 storage(std::in_place_index<1>, fst);
        return varerr::detail::storage_get<1>(storage).fst_;
    }() == ForwardCategory::ConstLValue);

    STATIC_REQUIRE([]{
        F0 storage(std::in_place_index<1>, 0);
        return varerr::detail::storage_get<1>(storage).fst_;
    }() == ForwardCategory::RValue);

    STATIC_REQUIRE([]{
        const int fst = 0;
        F0 storage(std::in_place_index<1>, std::move(fst)); // NOLINT
        return varerr::detail::storage_get<1>(storage).fst_;
    }() == ForwardCategory::ConstRValue);

    STATIC_REQUIRE([] -> std::pair<ForwardCategory, ForwardCategory> {
        const int snd = 1;
        F0 storage(std::in_place_index<1>, 0, snd);
        return { varerr::detail::storage_get<1>(storage).fst_, varerr::detail::storage_get<1>(storage).snd_ };
    }() == std::pair { ForwardCategory::RValue, ForwardCategory::ConstLValue });

    STATIC_REQUIRE([] -> std::pair<ForwardCategory, ForwardCategory> {
        const int fst = 0; int snd = 1;
        F0 storage(std::in_place_index<1>, std::move(fst), snd); // NOLINT
        return { varerr::detail::storage_get<1>(storage).fst_, varerr::detail::storage_get<1>(storage).snd_ };
    }() == std::pair { ForwardCategory::ConstRValue, ForwardCategory::LValue });

}

TEST_CASE("varerr_storage_constraints_storage_get", "[varerr][storage]") {

    constexpr std::size_t kStorageGetConstraintsTestBound = 3;
    constexpr std::size_t kStorageGetConstraintsTestIndices = 5;

    using S0 = S<kStorageGetConstraintsTestBound>;

    iterate_index_sequence<kStorageGetConstraintsTestIndices>([](const auto index) {
        constexpr std::size_t I = decltype(index)::value;
        iterate_cvref_matrix<S0>([&]<typename T>(const std::type_identity<T>) {
            if constexpr (I < varerr::detail::storage_size_v<S0>) {
                STATIC_REQUIRE(IsStorageGetWellFormed<T, I>);
            } else {
                STATIC_REQUIRE_FALSE(IsStorageGetWellFormed<T, I>);
            }
        });
    });

}

TEST_CASE("varerr_storage_constraints_storage_get_empty", "[varerr][storage]") {

    constexpr std::size_t kStorageGetConstraintsTestIndices = 5;

    iterate_index_sequence<kStorageGetConstraintsTestIndices>([](const auto index) {
        constexpr std::size_t I = decltype(index)::value;
        iterate_cvref_matrix<S<0>>([&]<typename T>(const std::type_identity<T>) {
            STATIC_REQUIRE_FALSE(IsStorageGetWellFormed<T, I>);
        });
    });

}

TEST_CASE("varerr_storage_constraints_storage_emplace", "[varerr][storage]") {

    constexpr std::size_t kStorageEmplaceConstraintsTestBound = 3;
    constexpr std::size_t kStorageEmplaceConstraintsTestIndices = 5;

    using S0 = S<kStorageEmplaceConstraintsTestBound>;

    iterate_index_sequence<kStorageEmplaceConstraintsTestIndices>([](const auto index) {
        constexpr std::size_t I = decltype(index)::value;
        iterate_cvref_matrix<S0>([&]<typename T>(const std::type_identity<T>) {

            // storage_emplace only accepts non-const, non-volatile lvalues.

            constexpr bool well_formed = I < varerr::detail::storage_size_v<S0> &&
                                         std::is_lvalue_reference_v<T> &&
                                         !std::is_const_v<std::remove_reference_t<T>> &&
                                         !std::is_volatile_v<std::remove_reference_t<T>>;

            if constexpr (well_formed) {
                STATIC_REQUIRE(IsStorageEmplaceWellFormed<T, I, E<I>>);
                STATIC_REQUIRE_FALSE(IsStorageEmplaceWellFormed<T, I, int*>);
            } else {
                STATIC_REQUIRE_FALSE(IsStorageEmplaceWellFormed<T, I, E<I>>);
            }

        });
    });

}

TEST_CASE("varerr_storage_constraints_storage_emplace_empty", "[varerr][storage]") {

    constexpr std::size_t kStorageEmplaceConstraintsTestIndices = 5;

    iterate_index_sequence<kStorageEmplaceConstraintsTestIndices>([](const auto index) {
        constexpr std::size_t I = decltype(index)::value;
        iterate_cvref_matrix<S<0>>([&]<typename T>(const std::type_identity<T>) {
            STATIC_REQUIRE_FALSE(IsStorageEmplaceWellFormed<T, I, int*>);
            STATIC_REQUIRE_FALSE(IsStorageEmplaceWellFormed<T, I, E<I>>);
        });
    });

}
