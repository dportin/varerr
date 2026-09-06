#include <catch2/catch_test_macros.hpp>

#include "include/lifetime.hpp"
#include "include/universe.hpp"

#include <varerr/status.hpp>
#include <varerr/result.hpp>


#include <algorithm>
#include <concepts>
#include <cstddef>
#include <functional>
#include <type_traits>
#include <utility>

using namespace varerr::tests::lifetime;
using namespace varerr::tests::universe;

// TODO: Remove this file once status.hpp and result.hpp tests refactored.

TEST_CASE("status_impl_static", "[status][impl][static]") {

    STATIC_REQUIRE_FALSE(std::is_constructible_v<varerr::BasicStatus<UniverseE>>);
    STATIC_REQUIRE_FALSE(std::is_default_constructible_v<varerr::BasicStatus<UniverseE>>);
    STATIC_REQUIRE_FALSE(std::is_default_constructible_v<varerr::BasicStatus<UniverseE, E<1>>>);

    STATIC_REQUIRE(std::is_trivially_destructible_v<varerr::BasicStatus<UniverseE>>);
    STATIC_REQUIRE(std::is_trivially_destructible_v<varerr::BasicStatus<UniverseE, E<1>>>);

    STATIC_REQUIRE(std::is_trivially_copyable_v<varerr::BasicStatus<UniverseE>>);
    STATIC_REQUIRE(std::is_trivially_copy_assignable_v<varerr::BasicStatus<UniverseE>>);
    STATIC_REQUIRE(std::is_trivially_copy_constructible_v<varerr::BasicStatus<UniverseE>>);
    STATIC_REQUIRE(std::is_trivially_move_assignable_v<varerr::BasicStatus<UniverseE>>);
    STATIC_REQUIRE(std::is_trivially_move_constructible_v<varerr::BasicStatus<UniverseE>>);

    STATIC_REQUIRE(std::is_trivially_copyable_v<varerr::BasicStatus<UniverseE, E<0>>>);
    STATIC_REQUIRE(std::is_trivially_copy_assignable_v<varerr::BasicStatus<UniverseE, E<0>>>);
    STATIC_REQUIRE(std::is_trivially_copy_constructible_v<varerr::BasicStatus<UniverseE, E<0>>>);
    STATIC_REQUIRE(std::is_trivially_move_assignable_v<varerr::BasicStatus<UniverseE, E<0>>>);
    STATIC_REQUIRE(std::is_trivially_move_constructible_v<varerr::BasicStatus<UniverseE, E<0>>>);

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
    using Si = varerr::BasicStatus<UniverseE, E<0>, E<1>, E<2>>;

    STATIC_REQUIRE(sizeof(Si) >= sizeof(St));
    STATIC_REQUIRE(alignof(Si) == std::max(alignof(std::size_t), alignof(St)));
    STATIC_REQUIRE(sizeof(Si) % alignof(Si) == 0);

    STATIC_REQUIRE(sizeof(Si) == sizeof(ExpectedLayout<E<0>, E<1>, E<2>>));
    STATIC_REQUIRE(alignof(Si) == alignof(ExpectedLayout<E<0>, E<1>, E<2>>));

}

TEST_CASE("status_impl_normalize", "[status][impl][functional]") {

    STATIC_REQUIRE(std::same_as<
        varerr::Status<UniverseE, E<0>, E<1>>,
        varerr::BasicStatus<UniverseE, E<0>, E<1>>
    >);

    STATIC_REQUIRE(std::same_as<
        varerr::Status<UniverseE, E<1>, E<0>>,
        varerr::BasicStatus<UniverseE, E<0>, E<1>>
    >);

    STATIC_REQUIRE(std::same_as<
        varerr::Status<UniverseE, E<0>, E<0>>,
        varerr::BasicStatus<UniverseE, E<0>>
    >);

    STATIC_REQUIRE(std::same_as<
        varerr::Status<UniverseE>,
        varerr::BasicStatus<UniverseE>
    >);

    STATIC_REQUIRE(std::same_as<
        varerr::Status<UniverseE, E<5>, E<2>, E<2>, E<8>, E<1>, E<2>, E<4>, E<2>>,
        varerr::BasicStatus<UniverseE, E<1>, E<2>, E<4>, E<5>, E<8>>
    >);

}

TEST_CASE("result_error_functional_deduction", "[result][error][functional]") {

    auto infer0 = varerr::Error(static_cast<short>(4));
    STATIC_REQUIRE(std::same_as<varerr::Error<short>, decltype(infer0)>);

    auto infer1 = varerr::Error(E<0> { 1 });
    STATIC_REQUIRE(std::same_as<varerr::Error<E<0>>, decltype(infer1)>);

    auto emplace0 = varerr::Error<E<0>> { E<0> { 42 } };
    STATIC_REQUIRE(std::same_as<varerr::Error<E<0>>, decltype(emplace0)>);
    REQUIRE(emplace0.unwrap().value_ == 42);

}

namespace {

template <typename T>
using R0 = varerr::Result<UniverseE, T>;

template <typename T>
using R1 = varerr::Result<UniverseE, T, E<0>>;

template <typename T>
using R2 = varerr::Result<UniverseE, T, E<0>, E<1>>;

template <typename T>
using R3 = varerr::Result<UniverseE, T, E<0>, E<1>, E<2>>;

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

    auto err0 = R2<int>(varerr::Error<E<1>>{ E<1> {42} }); // NOLINT
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
    R3<E<0>> res7 = res6.transform([](unsigned int n) -> E<0> { return E<0>(static_cast<std::size_t>(n)); });
    REQUIRE(res7.has_value());
    REQUIRE(!res7.has_error());
    REQUIRE(res7.value().value_ == 42);

    // result holds an error
    R3<unsigned int> err0 = R3<unsigned int>(varerr::Error<E<1>>(E<1>(42))); // NOLINT
    R3<unsigned int> err1 = err0.transform([](unsigned int n) -> unsigned int { return n + 1; });
    REQUIRE(err1.has_error());
    REQUIRE(!err1.has_value());
    REQUIRE(err1.holds_error<E<1>>());
    REQUIRE(err1.error<E<1>>().value_ == 42);

    // conversion when result holds an error
    R3<E<1>> err2 = err0.transform([](unsigned int n) -> E<1> { return E<1>(static_cast<std::size_t>(n)); });
    REQUIRE(err2.has_error());
    REQUIRE(!err2.has_value());
    REQUIRE(err2.holds_error<E<1>>());
    REQUIRE(err2.error<E<1>>().value_ == 42);

}

TEST_CASE("result_functional_transform_lifetime", "[result][functional]") {

    Record global {};
    using TrackedR = Tracked<int>;
    using TrackingR = varerr::Result<UniverseE, TrackedR, E<0>, E<1>, E<2>>;

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

TEST_CASE("result_functional_and_then", "[result][functional]") {


    using R0 = varerr::Result<UniverseE, short, E<1>, E<2>, E<3>>;
    using R1 = varerr::Result<UniverseE, int, E<0>, E<2>, E<4>>;

    R0 result0 = R0(5);
    auto result1 = result0.and_then([](short n) -> R1 {
        return R1(static_cast<int>(n) + 1);
    });

    REQUIRE(result1.has_value());
    REQUIRE(result1.value() == 6);

    STATIC_REQUIRE(std::same_as<
        decltype(result1),
        varerr::Result<UniverseE, int, E<0>, E<1>, E<2>, E<3>, E<4>>
    >);

}

TEST_CASE("result_functional_handle", "[result][functional]") {

    // Just checking the basic functionality...

    using R0 = varerr::Result<UniverseE, std::size_t>;
    using R1 = varerr::Result<UniverseE, std::size_t, E<0>>;

    R1 result0 = R1(varerr::Error<E<0>>(E<0>(42)));

    REQUIRE(result0.holds_error<E<0>>());
    REQUIRE(result0.error<E<0>>().value_ == 42);

    R1 handled0 = result0.handle<E<0>>([](const E<0>& e) -> R1 {
        return R1(e.value_);
    });

    REQUIRE(handled0.has_value());
    REQUIRE(handled0.value() == 42);

    R0 handled1 = result0.handle<E<0>>([](const E<0>& e) -> R0 {
        return R0(e.value_);
    });

    REQUIRE(handled1.has_value());
    REQUIRE(handled1.value() == 42);

}
