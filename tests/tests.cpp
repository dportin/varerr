#include <catch2/catch_test_macros.hpp>

#include <varerr/varset.hpp>
#include <varerr/status.hpp>
#include <varerr/result.hpp>

#include <cstddef>

#include <utility>
#include <concepts>
#include <algorithm>
#include <type_traits>

// Triviality

namespace {

    template <typename... Ts>
    using Storage = varerr::detail::Storage<Ts...>;

    using S0 = Storage<>;
    using S1 = Storage<short>;
    using S2 = Storage<short, int>;

    template <typename S>
    constexpr void varset_storage_trivial_test() {

        // Trivial copyability and destructibility are required.

        STATIC_REQUIRE(std::is_trivially_copyable_v<S>);
        STATIC_REQUIRE(std::is_trivially_destructible_v<S>);

        STATIC_REQUIRE(std::is_trivially_copy_constructible_v<S>);
        STATIC_REQUIRE(std::is_trivially_move_constructible_v<S>);
        STATIC_REQUIRE(std::is_trivially_copy_assignable_v<S>);
        STATIC_REQUIRE(std::is_trivially_move_assignable_v<S>);

        // The default constructor is non-trivial by construction.

        STATIC_REQUIRE_FALSE(std::is_trivially_default_constructible_v<S>);

    }

}

TEST_CASE("varset_storage_trivial", "[varset][storage][trivial]") {
    varset_storage_trivial_test<S0>();
    varset_storage_trivial_test<S1>();
    varset_storage_trivial_test<S2>();
}

// Storage and layout

namespace {

    template <typename... Ts>
    constexpr std::size_t max_sizeof_v = std::max({sizeof(Ts)...});

    template <typename... Ts>
    constexpr std::size_t max_alignof_v = std::max({alignof(Ts)...});

    template <typename... Ts>
    constexpr std::size_t max_sizeof_v<Storage<Ts...>> = max_sizeof_v<Ts...>;

    template <typename... Ts>
    constexpr std::size_t max_alignof_v<Storage<Ts...>> = max_alignof_v<Ts...>;

    template <typename S>
    constexpr void varset_storage_memory_test() {
        STATIC_REQUIRE(sizeof(S) == max_sizeof_v<S>);
        STATIC_REQUIRE(alignof(S) == max_alignof_v<S>);
    }

}

TEST_CASE("varset_storage_memory", "[varset][storage][memory]") {

    varset_storage_memory_test<S1>();
    varset_storage_memory_test<S2>();

    // The standard does not especify an exact value for the empty case.

    STATIC_REQUIRE(sizeof(Storage<>) > 0);
    STATIC_REQUIRE(sizeof(Storage<>) > 0);

}

namespace {

    template <typename S>
    constexpr void varset_storage_memory_layout_test() {
        STATIC_REQUIRE(std::is_standard_layout_v<Storage<S>>);
    }

}

TEST_CASE("varset_storage_memory_layout", "[varset][storage][memory]") {

    varset_storage_memory_layout_test<S0>();
    varset_storage_memory_layout_test<S1>();
    varset_storage_memory_layout_test<S2>();

    STATIC_REQUIRE(offsetof(S2, head_) == 0);
    STATIC_REQUIRE(offsetof(S2, tail_) == 0);
    STATIC_REQUIRE(offsetof(S2, tail_.head_) == 0);

}

namespace {

    struct TrivialStoreType {
        int x;
    };

    struct NonTrivialConstructType {
        int x;
        NonTrivialConstructType(int) {}
    };

    struct NonTrivialDestructType {
        int x;
        ~NonTrivialDestructType() {}
    };

    struct NonTrivialCopyType {
        int x;
        NonTrivialCopyType(const NonTrivialCopyType&) {}
    };

    struct NonTrivialMoveType {
        int x;
        NonTrivialMoveType(NonTrivialMoveType&&) {}
    };

}

TEST_CASE("varset_storage_trivial_store", "[varset][storage][trivial]") {

    STATIC_REQUIRE(varerr::detail::IsTriviallyStorable<int>);
    STATIC_REQUIRE(varerr::detail::IsTriviallyStorable<TrivialStoreType>);
    STATIC_REQUIRE(varerr::detail::IsTriviallyStorable<S2>);
    STATIC_REQUIRE(varerr::detail::IsTriviallyStorable<NonTrivialConstructType>);
    STATIC_REQUIRE(varerr::detail::IsTriviallyStorable<Storage<TrivialStoreType>>);
    STATIC_REQUIRE(varerr::detail::IsTriviallyStorable<Storage<NonTrivialConstructType>>);

    STATIC_REQUIRE_FALSE(varerr::detail::IsTriviallyStorable<const int>);
    STATIC_REQUIRE_FALSE(varerr::detail::IsTriviallyStorable<int&>);
    STATIC_REQUIRE_FALSE(varerr::detail::IsTriviallyStorable<void>);
    STATIC_REQUIRE_FALSE(varerr::detail::IsTriviallyStorable<bool(int)>);

    STATIC_REQUIRE_FALSE(varerr::detail::IsTriviallyStorable<NonTrivialDestructType>);
    STATIC_REQUIRE_FALSE(varerr::detail::IsTriviallyStorable<NonTrivialCopyType>);
    STATIC_REQUIRE_FALSE(varerr::detail::IsTriviallyStorable<NonTrivialMoveType>);

}

TEST_CASE("varset_storage_functional", "[varset][storage][functional]") {

    SECTION("replace_head") {

        STATIC_REQUIRE([]{
            S2 storage(std::in_place_index<0>, static_cast<short>(100));
            return storage.head_;
        }() == 100);

        STATIC_REQUIRE([]{
            S2 storage(std::in_place_index<1>, 100);
            std::construct_at(std::addressof(storage.head_), static_cast<short>(101));
            return storage.head_;
        }() == 101);

    }

    SECTION("replace_tail") {

        STATIC_REQUIRE([]{
            S2 storage(std::in_place_index<1>, 100);
            return storage.tail_.head_;
        }() == 100);

        STATIC_REQUIRE([]{
            S2 storage(std::in_place_index<0>, static_cast<short>(100));
            std::construct_at(std::addressof(storage.tail_));
            std::construct_at(std::addressof(storage.tail_.head_), 101);
            return storage.tail_.head_;
        }() == 101);

    }

}
