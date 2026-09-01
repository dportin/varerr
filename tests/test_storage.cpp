#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_template_test_macros.hpp>

#include "include/utilities.hpp"
#include "include/universe.hpp"

#include <varerr/storage.hpp>

#include <cstddef>
#include <tuple>
#include <type_traits>

using namespace varerr::tests;
using namespace varerr::tests::universe;

namespace {

// Lift the test universe to the storage class.

template <std::size_t N>
using S = lift_index_sequence_t<varerr::detail::Storage, E, N>;

static_assert(std::same_as<S<0>, varerr::detail::Storage<>>);
static_assert(std::same_as<S<1>, varerr::detail::Storage<E<0>>>);
static_assert(std::same_as<S<2>, varerr::detail::Storage<E<0>, E<1>>>);

// Round the size of the storage alternatives up to their maximum alignment.

template <typename... Es>
constexpr std::size_t storage_sizeof_v = round_to_multiple(max_sizeof_v<Es...>, max_alignof_v<Es...>);

struct storage_sizeof_fun {
    template <typename... Es>
    using apply = std::integral_constant<std::size_t, storage_sizeof_v<Es...>>;
};

} // namespace

TEMPLATE_TEST_CASE("varerr_storage_trivial", "[varerr][storage]", S<0>, S<1>, S<2>, S<3>) {

    // The alternatives must be trivially copyable and destructible.

    STATIC_REQUIRE(std::is_trivially_copyable_v<TestType>);
    STATIC_REQUIRE(std::is_trivially_destructible_v<TestType>);

    STATIC_REQUIRE(std::is_trivially_copy_constructible_v<TestType>);
    STATIC_REQUIRE(std::is_trivially_move_constructible_v<TestType>);
    STATIC_REQUIRE(std::is_trivially_copy_assignable_v<TestType>);
    STATIC_REQUIRE(std::is_trivially_move_assignable_v<TestType>);

    // The default constructor is explicitly defined and thus non-trivial.

    STATIC_REQUIRE(std::is_default_constructible_v<TestType>);
    STATIC_REQUIRE_FALSE(std::is_trivially_default_constructible_v<TestType>);

}

TEST_CASE("varerr_storage_memory_empty", "[varerr][storage]") {

    // The standard only mandates that the size and alignment is greater than
    // zero in the empty case.

    STATIC_REQUIRE(sizeof(S<0>) == 1);
    STATIC_REQUIRE(alignof(S<0>) == 1);

}

TEMPLATE_TEST_CASE("varerr_storage_memory_homogeneous", "[varerr][storage]", S<1>, S<2>, S<3>) {

    STATIC_REQUIRE(sizeof(TestType) == pack_apply_v<storage_sizeof_fun, TestType>);
    STATIC_REQUIRE(alignof(TestType) == pack_apply_v<max_alignof_fun, TestType>);

}

TEST_CASE("varerr_storage_memory_heterogeneous", "[varerr][storage]") {

    struct alignas(32) TestStruct {
        char bytes_[40];
    };

    using TestType = varerr::detail::Storage<char, TestStruct, double>;

    STATIC_REQUIRE(sizeof(TestType) == pack_apply_v<storage_sizeof_fun, TestType>);

}

namespace {

template <typename S>
constexpr void varset_storage_memory_layout_test() {

    STATIC_REQUIRE(std::is_standard_layout_v<S>);

}

} // namespace

TEST_CASE("varset_storage_layout", "[varset][storage]") {

    varset_storage_memory_layout_test<S<0>>();
    varset_storage_memory_layout_test<S<1>>();
    varset_storage_memory_layout_test<S<2>>();

    STATIC_REQUIRE(offsetof(S<2>, head_) == 0);
    STATIC_REQUIRE(offsetof(S<2>, tail_) == 0);
    STATIC_REQUIRE(offsetof(S<2>, tail_.head_) == 0);

}

namespace {

    struct TrivialStoreType {
        int store_;
    };

    struct NonTrivialConstructType {
        int store_;
        NonTrivialConstructType(int) {}
    };


    struct NonTrivialDestructType {
        int store_;
        ~NonTrivialDestructType() {}
    };

    struct NonTrivialCopyType {
        int store_;
        NonTrivialCopyType(const NonTrivialCopyType&) {}
    };

    struct NonTrivialMoveType {
        int store_;
        NonTrivialMoveType(NonTrivialMoveType&&) noexcept {}
    };

} // namespace

TEST_CASE("varset_storage_trivial_store", "[varset][storage]") {

    STATIC_REQUIRE(varerr::IsTriviallyStorable<int>);
    STATIC_REQUIRE(varerr::IsTriviallyStorable<TrivialStoreType>);
    STATIC_REQUIRE(varerr::IsTriviallyStorable<S<2>>);
    STATIC_REQUIRE(varerr::IsTriviallyStorable<NonTrivialConstructType>);
    STATIC_REQUIRE(varerr::IsTriviallyStorable<varerr::detail::Storage<TrivialStoreType>>);
    STATIC_REQUIRE(varerr::IsTriviallyStorable<varerr::detail::Storage<NonTrivialConstructType>>);

    STATIC_REQUIRE_FALSE(varerr::IsTriviallyStorable<const int>);
    STATIC_REQUIRE_FALSE(varerr::IsTriviallyStorable<int&>);
    STATIC_REQUIRE_FALSE(varerr::IsTriviallyStorable<void>);
    STATIC_REQUIRE_FALSE(varerr::IsTriviallyStorable<bool(int)>);

    STATIC_REQUIRE_FALSE(varerr::IsTriviallyStorable<NonTrivialDestructType>);
    STATIC_REQUIRE_FALSE(varerr::IsTriviallyStorable<NonTrivialCopyType>);
    STATIC_REQUIRE_FALSE(varerr::IsTriviallyStorable<NonTrivialMoveType>);

}

TEST_CASE("varset_storage_construct", "[varset][storage]") {

    SECTION("replace_head") {

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

    SECTION("replace_tail") {

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

}
