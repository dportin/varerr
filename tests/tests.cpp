#include <catch2/catch_test_macros.hpp>

#include "include/lifetime.hpp"

#include <varerr/status.hpp>
#include <varerr/result.hpp>

#include <cstddef>

#include <algorithm>
#include <concepts>
#include <type_traits>
#include <utility>
#include <functional>

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

    STATIC_REQUIRE(sizeof(Storage<>) > 0); // NOLINT
    STATIC_REQUIRE(sizeof(Storage<>) > 0); // NOLINT

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

    // NOLINTBEGIN

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
        NonTrivialMoveType(NonTrivialMoveType&&) {}
    };

    // NOLINTEND

}

TEST_CASE("varset_storage_trivial_store", "[varset][storage][trivial]") {

    STATIC_REQUIRE(varerr::IsTriviallyStorable<int>);
    STATIC_REQUIRE(varerr::IsTriviallyStorable<TrivialStoreType>);
    STATIC_REQUIRE(varerr::IsTriviallyStorable<S2>);
    STATIC_REQUIRE(varerr::IsTriviallyStorable<NonTrivialConstructType>);
    STATIC_REQUIRE(varerr::IsTriviallyStorable<Storage<TrivialStoreType>>);
    STATIC_REQUIRE(varerr::IsTriviallyStorable<Storage<NonTrivialConstructType>>);

    STATIC_REQUIRE_FALSE(varerr::IsTriviallyStorable<const int>);
    STATIC_REQUIRE_FALSE(varerr::IsTriviallyStorable<int&>);
    STATIC_REQUIRE_FALSE(varerr::IsTriviallyStorable<void>);
    STATIC_REQUIRE_FALSE(varerr::IsTriviallyStorable<bool(int)>);

    STATIC_REQUIRE_FALSE(varerr::IsTriviallyStorable<NonTrivialDestructType>);
    STATIC_REQUIRE_FALSE(varerr::IsTriviallyStorable<NonTrivialCopyType>);
    STATIC_REQUIRE_FALSE(varerr::IsTriviallyStorable<NonTrivialMoveType>);

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
            return storage.tail_.head_; // NOLINT
        }() == 100);

        STATIC_REQUIRE([]{
            S2 storage(std::in_place_index<0>, static_cast<short>(100));
            std::construct_at(std::addressof(storage.tail_));
            std::construct_at(std::addressof(storage.tail_.head_), 101); // NOLINT
            return storage.tail_.head_; // NOLINT
        }() == 101);

    }

}

namespace {

    template <std::size_t N>
    struct E {
        
        std::size_t value_;

        constexpr E(std::size_t n) noexcept :
            value_(n) {}

        template <std::size_t>
        friend struct E;
    
    };

    template <std::size_t N, std::size_t M>
    constexpr bool operator==(const E<N>& lhs, const E<M>& rhs) noexcept {
        return lhs.value_ == rhs.value_;
    }

    struct Universe {

        template <typename T>
        struct rank_trait;

        template <std::size_t N>
        struct rank_trait<E<N>> {
            static constexpr std::size_t rank_ = N;
        };

        template <typename T>
        requires requires { rank_trait<T>::rank_; }
        static constexpr std::size_t rank = rank_trait<T>::rank_;

    };

}

TEST_CASE("varset_ranked", "[varset][row][ranked]") {

    STATIC_REQUIRE(varerr::detail::IsRankedPack<Universe>);
    STATIC_REQUIRE(varerr::detail::IsRankedPack<Universe, E<0>>);
    STATIC_REQUIRE(varerr::detail::IsRankedPack<Universe, E<1>, E<0>>);

    STATIC_REQUIRE_FALSE(varerr::detail::IsRankedPack<Universe, int>);
    STATIC_REQUIRE_FALSE(varerr::detail::IsRankedPack<Universe, E<1>, int>);

    STATIC_REQUIRE(varerr::IsRankedRow<Universe, varerr::Row<>>);
    STATIC_REQUIRE(varerr::IsRankedRow<Universe, varerr::Row<E<0>>>);
    STATIC_REQUIRE(varerr::IsRankedRow<Universe, varerr::Row<E<1>, E<0>>>);

    STATIC_REQUIRE_FALSE(varerr::IsRanked<Universe, varerr::Row<int>>);
    STATIC_REQUIRE_FALSE(varerr::IsRanked<Universe, varerr::Row<E<1>, int>>);

}

TEST_CASE("varset_normalized", "[varset][row][normalized]") {

    STATIC_REQUIRE(varerr::detail::IsNormalizedPack<Universe>);
    STATIC_REQUIRE(varerr::detail::IsNormalizedPack<Universe, E<0>>);
    STATIC_REQUIRE(varerr::detail::IsNormalizedPack<Universe, E<0>, E<1>>);
    STATIC_REQUIRE(varerr::detail::IsNormalizedPack<Universe, E<0>, E<1>, E<2>>);

    STATIC_REQUIRE_FALSE(varerr::detail::IsNormalizedPack<Universe, E<1>, E<0>>);
    STATIC_REQUIRE_FALSE(varerr::detail::IsNormalizedPack<Universe, E<0>, E<2>, E<1>>);
    STATIC_REQUIRE_FALSE(varerr::detail::IsNormalizedPack<Universe, E<0>, E<1>, int, E<2>>);

    STATIC_REQUIRE(varerr::IsNormalizedRow<Universe, varerr::Row<>>);
    STATIC_REQUIRE(varerr::IsNormalizedRow<Universe, varerr::Row<E<0>>>);
    STATIC_REQUIRE(varerr::IsNormalizedRow<Universe, varerr::Row<E<0>, E<1>>>);
    STATIC_REQUIRE(varerr::IsNormalizedRow<Universe, varerr::Row<E<0>, E<1>, E<2>>>);

    STATIC_REQUIRE_FALSE(varerr::IsNormalizedRow<Universe, varerr::Row<E<1>, E<0>>>);
    STATIC_REQUIRE_FALSE(varerr::IsNormalizedRow<Universe, varerr::Row<E<0>, E<2>, E<1>>>);
    STATIC_REQUIRE_FALSE(varerr::IsNormalizedRow<Universe, varerr::Row<E<0>, E<1>, int, E<2>>>);

}

TEST_CASE("status_impl_static", "[status][impl][static]") {

    STATIC_REQUIRE_FALSE(std::is_constructible_v<varerr::detail::StatusImpl<Universe>>);
    STATIC_REQUIRE_FALSE(std::is_default_constructible_v<varerr::detail::StatusImpl<Universe>>);
    STATIC_REQUIRE_FALSE(std::is_default_constructible_v<varerr::detail::StatusImpl<Universe, E<1>>>);

    STATIC_REQUIRE(std::is_trivially_destructible_v<varerr::detail::StatusImpl<Universe>>);
    STATIC_REQUIRE(std::is_trivially_destructible_v<varerr::detail::StatusImpl<Universe, E<1>>>);

    STATIC_REQUIRE(std::is_trivially_copyable_v<varerr::detail::StatusImpl<Universe>>);
    STATIC_REQUIRE(std::is_trivially_copy_assignable_v<varerr::detail::StatusImpl<Universe>>);
    STATIC_REQUIRE(std::is_trivially_copy_constructible_v<varerr::detail::StatusImpl<Universe>>);
    STATIC_REQUIRE(std::is_trivially_move_assignable_v<varerr::detail::StatusImpl<Universe>>);
    STATIC_REQUIRE(std::is_trivially_move_constructible_v<varerr::detail::StatusImpl<Universe>>);

    STATIC_REQUIRE(std::is_trivially_copyable_v<varerr::detail::StatusImpl<Universe, E<0>>>);
    STATIC_REQUIRE(std::is_trivially_copy_assignable_v<varerr::detail::StatusImpl<Universe, E<0>>>);
    STATIC_REQUIRE(std::is_trivially_copy_constructible_v<varerr::detail::StatusImpl<Universe, E<0>>>);
    STATIC_REQUIRE(std::is_trivially_move_assignable_v<varerr::detail::StatusImpl<Universe, E<0>>>);
    STATIC_REQUIRE(std::is_trivially_move_constructible_v<varerr::detail::StatusImpl<Universe, E<0>>>);

}

namespace {

template <typename... Ts>
struct ExpectedLayout final {
    std::size_t active;
    varerr::detail::Storage<Ts...> alternatives;
};

} // namespace

TEST_CASE("status_impl_memory", "[status][impl][memory]") {

    using St = varerr::detail::Storage<E<0>, E<1>, E<2>>;
    using Si = varerr::detail::StatusImpl<Universe, E<0>, E<1>, E<2>>;

    STATIC_REQUIRE(sizeof(Si) >= sizeof(St));
    STATIC_REQUIRE(alignof(Si) == std::max(alignof(std::size_t), alignof(St)));
    STATIC_REQUIRE(sizeof(Si) % alignof(Si) == 0);

    STATIC_REQUIRE(sizeof(Si) == sizeof(ExpectedLayout<E<0>, E<1>, E<2>>));
    STATIC_REQUIRE(alignof(Si) == alignof(ExpectedLayout<E<0>, E<1>, E<2>>));

}

TEST_CASE("status_impl_normalize", "[status][impl][functional]") {

    STATIC_REQUIRE(std::same_as<
        varerr::detail::Status<Universe, E<0>, E<1>>,
        varerr::detail::StatusImpl<Universe, E<0>, E<1>>
    >);

    STATIC_REQUIRE(std::same_as<
        varerr::detail::Status<Universe, E<1>, E<0>>,
        varerr::detail::StatusImpl<Universe, E<0>, E<1>>
    >);

    STATIC_REQUIRE(std::same_as<
        varerr::detail::Status<Universe, E<0>, E<0>>,
        varerr::detail::StatusImpl<Universe, E<0>>
    >);

    STATIC_REQUIRE(std::same_as<
        varerr::detail::Status<Universe>,
        varerr::detail::StatusImpl<Universe>
    >);

    STATIC_REQUIRE(std::same_as<
        varerr::detail::Status<Universe, E<5>, E<2>, E<2>, E<8>, E<1>, E<2>, E<4>, E<2>>,
        varerr::detail::StatusImpl<Universe, E<1>, E<2>, E<4>, E<5>, E<8>>
    >);

}

TEST_CASE("result_error_functional_deduction", "[result][error][functional]") {
    
    auto infer0 = varerr::Error(static_cast<short>(4));
    STATIC_REQUIRE(std::same_as<varerr::Error<short>, decltype(infer0)>);

    auto infer1 = varerr::Error(E<0> { 1 });
    STATIC_REQUIRE(std::same_as<varerr::Error<E<0>>, decltype(infer1)>);

    auto emplace0 = varerr::Error<E<0>> { std::size_t { 42 } }; // NOLINT
    STATIC_REQUIRE(std::same_as<varerr::Error<E<0>>, decltype(emplace0)>);
    REQUIRE(emplace0.unwrap().value_ == 42);

}

namespace {

template <typename T>
using R0 = varerr::Result<Universe, T>;

template <typename T>
using R1 = varerr::Result<Universe, T, E<0>>;

template <typename T>
using R2 = varerr::Result<Universe, T, E<0>, E<1>>;

template <typename T>
using R3 = varerr::Result<Universe, T, E<0>, E<1>, E<2>>;

} // namespace

TEST_CASE("result_functional_access", "[result][functional]") {

    auto res0 = R0<int>(42); // NOLINT
    REQUIRE(res0.has_value());
    REQUIRE(!res0.has_error());
    REQUIRE(res0.value() == 42);
    REQUIRE(res0.value_if() != nullptr);
    REQUIRE(*res0.value_if() == 42);
    REQUIRE(!res0.holds_error<E<0>>());

    auto res1 = R2<int>(42); // NOLINT
    REQUIRE(res1.has_value());
    REQUIRE(!res1.has_error());
    REQUIRE(res1.value() == 42);
    REQUIRE(res1.value_if() != nullptr);
    REQUIRE(*res1.value_if() == 42);
    REQUIRE(!res1.holds_error<E<1>>());
    REQUIRE(!res1.holds_error<E<99>>());

    auto err0 = R2<int>(varerr::Error<E<1>>(42)); // NOLINT
    REQUIRE(!err0.has_value());
    REQUIRE(err0.has_error());
    REQUIRE(err0.holds_error<E<1>>());
    REQUIRE(!err0.holds_error<E<0>>());
    REQUIRE(err0.value_if() == nullptr);
    REQUIRE(err0.error<E<1>>().value_ == 42);
    REQUIRE(err0.error_if<E<0>>() == nullptr);
    REQUIRE(err0.error_if<E<1>>()->value_ == 42);

}

TEST_CASE("result_functional_transform", "[result][functional]") {

    const auto increment = std::bind_back(std::plus<> {}, 1);

    // transform
    R3<int> res0 = R3<int>(42); // NOLINT
    R3<int> res1 = res0.transform(increment);
    REQUIRE(res1.value() == 43);

    // chaining
    R3<int> res2 = res0.transform(increment).transform(increment);
    REQUIRE(res2.value() == 44);

    // conversion
    R3<long> res3 = res0.transform([](int n) -> long { return n + 1; });
    REQUIRE(res3.value() == 43);

    // conversion chaining
    R3<short> res4 = R3<short>(42); // NOLINT
    R3<long> res5 = res4.transform([](short n) -> int { return n + 1; })
                        .transform([](int n) -> long { return n + 1; });
    REQUIRE(res5.value() == 44);

    // transform dispatches to correct constructor when value in error row
    R3<unsigned int> res6 = R3<unsigned int>(42); // NOLINT
    R3<E<0>> res7 = res6.transform([](unsigned int n) -> E<0> { return n; });
    REQUIRE(res7.has_value());
    REQUIRE(!res7.has_error());
    REQUIRE(res7.value().value_ == 42);

    // result holds an error
    R3<unsigned int> err0 = R3<unsigned int>(varerr::Error<E<1>>(42)); // NOLINT
    R3<unsigned int> err1 = err0.transform([](unsigned int n) -> unsigned int { return n + 1; });
    REQUIRE(err1.has_error());
    REQUIRE(!err1.has_value());
    REQUIRE(err1.holds_error<E<1>>());
    REQUIRE(err1.error<E<1>>().value_ == 42);

    // conversion when result holds an error
    R3<E<1>> err2 = err0.transform([](unsigned int n) -> E<1> { return n; });
    REQUIRE(err2.has_error());
    REQUIRE(!err2.has_value());
    REQUIRE(err2.holds_error<E<1>>());
    REQUIRE(err2.error<E<1>>().value_ == 42);
    
}

TEST_CASE("result_functional_transform_lifetime", "[result][functional]") {

    lifetime::Counts global {};
    using TrackedR = lifetime::Tracked<int>;
    using TrackingR = varerr::Result<Universe, TrackedR, E<0>, E<1>, E<2>>;

    TrackingR tracked0 = TrackingR(TrackedR(&global, 5)); // NOLINT

    REQUIRE(tracked0.value().local().copy_constructed == 0);
    REQUIRE(tracked0.value().local().move_constructed == 1);
    
    TrackingR tracked1 = tracked0.transform([](TrackedR& tracked) { tracked.value()++; return tracked; });
    
    REQUIRE(tracked1.value().local().copy_constructed == 1); /* parameter is an lvalue */
    REQUIRE(tracked1.value().local().move_constructed == 2); /* result is moved */

    TrackingR tracked2 = std::move(tracked1).transform([](TrackedR&& tracked) { std::move(tracked).value()++; return tracked; });

    REQUIRE(tracked2.value().local().copy_constructed == 1);
    REQUIRE(tracked2.value().local().move_constructed == 4); /* parameter is an rvalue and result is moved */

    TrackingR tracked3 = tracked2.transform([](TrackedR& tracked) { tracked.value()++; return tracked; })
                                 .transform([](TrackedR&& tracked) { std::move(tracked).value()++; return tracked; })
                                 .transform([](TrackedR&& tracked) { std::move(tracked).value()++; return tracked; });

    REQUIRE(tracked3.value().local().copy_constructed == 2);
    REQUIRE(tracked3.value().local().move_constructed == 9);

}

// TODO: Enumerate all combinations from a small universe E<0>, E<1>, ...

TEST_CASE("row_operations_spot_test", "[row]") {
   
    STATIC_REQUIRE(std::same_as<
        varerr::row_union_normalized_t<Universe,
            varerr::Row<E<0>>,
            varerr::Row<E<1>>
        >,
        varerr::Row<E<0>, E<1>>
    >);

    STATIC_REQUIRE(std::same_as<
        varerr::row_union_normalized_t<Universe,
            varerr::Row<E<0>, E<1>, E<7>>,
            varerr::Row<E<1>, E<5>, E<7>, E<9>>
        >,
        varerr::Row<E<0>, E<1>, E<5>, E<7>, E<9>>
    >);

}

TEST_CASE("result_functional_and_then", "[result][functional]") {


    using R0 = varerr::Result<Universe, short, E<1>, E<2>, E<3>>;
    using R1 = varerr::Result<Universe, int, E<0>, E<2>, E<4>>;

    R0 result0 = R0(5);
    auto result1 = result0.and_then([](short n) -> R1 {
        return R1(static_cast<int>(n) + 1);
    });

    REQUIRE(result1.has_value());
    REQUIRE(result1.value() == 6);
    
    STATIC_REQUIRE(std::same_as<
        decltype(result1),
        varerr::Result<Universe, int, E<0>, E<1>, E<2>, E<3>, E<4>>
    >);
    
}
