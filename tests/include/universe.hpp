#ifndef VARERR_TESTS_UNIVERSE_HPP
#define VARERR_TESTS_UNIVERSE_HPP

#include "utilities.hpp"

#include <algorithm>
#include <cassert>
#include <climits>
#include <compare>
#include <cstddef>
#include <span>
#include <type_traits>

namespace varerr::tests::universe {

// Recover a type from its rank. IWYU (0.26) crashes when parsing a dependent
// template member of a template type parameter and unrank is not used in the
// main source files, so the definition lives here (away from the IWYU tests).

template <typename M, std::size_t N>
concept IsRankInvertible = requires {
    typename M::template unrank<N>;
};

template <typename M, std::size_t N>
requires IsRankInvertible<M, N>
using unrank_v = M::template unrank<N>;

// Homogeneous universe of trivially storable types parameterized by rank.

template <std::size_t N>
struct E {

    std::size_t value_ {};

    constexpr E() noexcept = default;

    constexpr explicit E(std::size_t value) noexcept : value_{value} {}

    [[nodiscard]] constexpr std::size_t value() const noexcept {
        return this->value_;
    }

};

template <std::size_t N, std::size_t M>
[[nodiscard]] constexpr bool operator==(const E<N>& lhs, const E<M>& rhs) noexcept {
    return N == M && lhs.value() == rhs.value();
}

template <std::size_t N, std::size_t M>
[[nodiscard]] constexpr std::strong_ordering operator<=>(const E<N>& lhs, const E<M>& rhs) noexcept {

    constexpr std::strong_ordering order = N <=> M;
    if constexpr (order != std::strong_ordering::equal) {
        return order;
    }

    return lhs.value() <=> rhs.value();

}

struct UniverseE {

    // The rank trait strips cvref-qualifiers for testing the algebra traits.

    template <typename T>
    struct rank_trait;

    template <std::size_t N>
    struct rank_trait<E<N>> : std::integral_constant<std::size_t, N> {};

    template <typename T>
    requires requires { rank_trait<std::remove_cvref_t<T>>::value; }
    static constexpr std::size_t rank = rank_trait<std::remove_cvref_t<T>>::value;

    template <std::size_t N>
    using unrank = E<N>;

};

static_assert(std::same_as<UniverseE::template unrank<UniverseE::template rank<E<0>>>, E<0>>);
static_assert(std::same_as<UniverseE::template unrank<UniverseE::template rank<E<1>>>, E<1>>);
static_assert(std::same_as<UniverseE::template unrank<UniverseE::template rank<E<2>>>, E<2>>);

// The homogeneous universe with a rank function that differentiates const and
// non-const elements.

template <typename T>
using const_preserving_decay_t = std::remove_volatile_t<std::remove_reference_t<T>>;

struct ConstUniverseE {

    template <typename T>
    struct rank_trait;

    template <std::size_t N>
    struct rank_trait<E<N>> : std::integral_constant<std::size_t, 2 * N> {};

    template <std::size_t N>
    struct rank_trait<const E<N>> : std::integral_constant<std::size_t, 2 * N + 1> {};

    template <typename T>
    requires requires { rank_trait<const_preserving_decay_t<T>>::value; }
    static constexpr std::size_t rank = rank_trait<const_preserving_decay_t<T>>::value;

    template <std::size_t Z>
    using unrank = std::conditional_t<Z % 2 == 0, std::type_identity<E<Z / 2>>, std::type_identity<const E<Z / 2>>>::type;

};

static_assert(std::same_as<ConstUniverseE::template unrank<ConstUniverseE::template rank<E<0>>>, E<0>>);
static_assert(std::same_as<ConstUniverseE::template unrank<ConstUniverseE::template rank<const E<0>>>, const E<0>>);
static_assert(std::same_as<ConstUniverseE::template unrank<ConstUniverseE::template rank<E<1>>>, E<1>>);
static_assert(std::same_as<ConstUniverseE::template unrank<ConstUniverseE::template rank<const E<1>>>, const E<1>>);

// Heterogeneous universe of trivially storable types parameterized by payload
// size and log-alignment.

inline constexpr std::size_t kMaxLogAlign = 13; /* 2^13 = 8192 */

template <std::size_t N, std::size_t A>
struct H;

template <std::size_t A>
struct alignas((std::size_t {1} << A)) H<0, A> {

    static_assert(A <= kMaxLogAlign, "A must not exceed kMaxLogAlign");

    constexpr H() noexcept = default;

    constexpr explicit H(std::size_t) noexcept {}

    [[nodiscard]] constexpr std::span<const unsigned char> bytes() const noexcept {
        return {};
    }

};

template <std::size_t N, std::size_t A>
struct H {

    static_assert(A <= kMaxLogAlign, "A must not exceed kMaxLogAlign");

    alignas((std::size_t {1} << A)) unsigned char value_[N] {}; // NOLINT

    constexpr H() noexcept = default;

    constexpr explicit H(std::size_t value) noexcept {
        for (std::size_t i = 0; i < N; ++i) {
            value_[i] = static_cast<unsigned char>(
                value >> (CHAR_BIT * (i % sizeof(std::size_t)))
            );
        }
    }

    [[nodiscard]] constexpr std::span<const unsigned char> bytes() const noexcept {
        return std::span(this->value_, N);
    }

};

template <std::size_t N, std::size_t A, std::size_t M, std::size_t B>
[[nodiscard]] constexpr bool operator==(const H<N, A>& lhs, const H<M, B>& rhs) noexcept {
    return N == M && A == B && std::ranges::equal(lhs.bytes(), rhs.bytes());

}

template <std::size_t N, std::size_t A, std::size_t M, std::size_t B>
[[nodiscard]] constexpr std::strong_ordering operator<=>(const H<N, A>& lhs, const H<M, B>& rhs) noexcept {

    constexpr std::strong_ordering size_order = N <=> M;
    if constexpr (size_order != std::strong_ordering::equal) {
        return size_order;
    }

    constexpr std::strong_ordering align_order = A <=> B;
    if constexpr (align_order != std::strong_ordering::equal) {
        return align_order;
    }

    return std::lexicographical_compare_three_way(
        lhs.bytes().begin(), lhs.bytes().end(), rhs.bytes().begin(), rhs.bytes().end()
    );

}

struct UniverseH {

    template <typename T>
    struct rank_trait;

    // The rank trait strips cvref-qualifiers for testing the algebra traits.

    template <std::size_t N, std::size_t A>
    struct rank_trait<H<N, A>> : std::integral_constant<std::size_t, N * (kMaxLogAlign + 1) + A> {};

    template <typename T>
    requires requires { rank_trait<std::remove_cvref_t<T>>::value; }
    static constexpr std::size_t rank = rank_trait<std::remove_cvref_t<T>>::value;

    template <std::size_t Z>
    using unrank = H<Z / (kMaxLogAlign + 1), Z % (kMaxLogAlign + 1)>;

};

static_assert(std::same_as<UniverseH::template unrank<UniverseH::template rank<H<1,1>>>, H<1,1>>);
static_assert(std::same_as<UniverseH::template unrank<UniverseH::template rank<H<2,2>>>, H<2,2>>);
static_assert(std::same_as<UniverseH::template unrank<UniverseH::template rank<H<4,4>>>, H<4,4>>);

// Universe with a single rank collision.

struct AliasA { int value_; };
struct AliasB { int value_; };
struct AliasC { int value_; };

struct UniverseAlias {

    template <typename T>
    struct rank_trait;

    template <typename T>
    requires requires { rank_trait<std::remove_cvref_t<T>>::value; }
    static constexpr std::size_t rank = rank_trait<std::remove_cvref_t<T>>::value;

};

template <>
struct UniverseAlias::rank_trait<AliasA> : std::integral_constant<std::size_t, 0> {};

template <>
struct UniverseAlias::rank_trait<AliasB> : std::integral_constant<std::size_t, 0> {};

template <>
struct UniverseAlias::rank_trait<AliasC> : std::integral_constant<std::size_t, 1> {};

// Assorted types designed to break specific invariants.

// Entirely trivial type.

struct TrivialType {
    int value_;
};

static_assert(std::is_trivial_v<TrivialType>);
static_assert(std::is_default_constructible_v<TrivialType>);

// Degenerate value type.

struct DegenerateValueType {
    int value_;
    DegenerateValueType() = delete;
    explicit DegenerateValueType(int value) noexcept(false) : value_(value) {}
    DegenerateValueType(const DegenerateValueType&) = delete;
    DegenerateValueType(DegenerateValueType&&) = delete;
    DegenerateValueType& operator=(const DegenerateValueType&) = delete;
    DegenerateValueType& operator=(DegenerateValueType&&) = delete;
};

struct DegenerateErrorType {
    int value_;
    DegenerateErrorType() = delete;
    explicit DegenerateErrorType(int value) noexcept(false) : value_(value) {}
    DegenerateErrorType(const DegenerateErrorType&) noexcept = default;
    DegenerateErrorType(DegenerateErrorType&&) noexcept = default;
    DegenerateErrorType& operator=(const DegenerateErrorType&) = delete;
    DegenerateErrorType& operator=(DegenerateErrorType&&) = delete;
};

// DegenerateErrorType has a deleted default constructor and is therefore not
// trivial. Clang (16.2.0) incorrectly reports that the class is trivial.

static_assert(std::is_trivially_copyable_v<DegenerateErrorType>);
static_assert(std::is_trivially_destructible_v<DegenerateErrorType>);
static_assert(std::is_trivially_copy_constructible_v<DegenerateErrorType>);
static_assert(std::is_trivially_move_constructible_v<DegenerateErrorType>);
static_assert(!std::is_trivially_copy_assignable_v<DegenerateErrorType>);
static_assert(!std::is_trivially_move_assignable_v<DegenerateErrorType>);
static_assert(!std::is_trivially_default_constructible_v<DegenerateErrorType>);

static_assert(std::is_destructible_v<DegenerateErrorType>);
static_assert(std::is_copy_constructible_v<DegenerateErrorType>);
static_assert(std::is_move_constructible_v<DegenerateErrorType>);
static_assert(!std::is_copy_assignable_v<DegenerateErrorType>);
static_assert(!std::is_move_assignable_v<DegenerateErrorType>);
static_assert(!std::is_default_constructible_v<DegenerateErrorType>);

static_assert(std::is_nothrow_destructible_v<DegenerateErrorType>);
static_assert(std::is_nothrow_copy_constructible_v<DegenerateErrorType>);
static_assert(std::is_nothrow_move_constructible_v<DegenerateErrorType>);
static_assert(!std::is_nothrow_constructible_v<DegenerateErrorType, int>);
static_assert(!std::is_nothrow_default_constructible_v<DegenerateErrorType>);
static_assert(!std::is_nothrow_copy_assignable_v<DegenerateErrorType>);
static_assert(!std::is_nothrow_move_assignable_v<DegenerateErrorType>);

// Trivially copy assignable but not copy constructible.

struct NoCopyConstructType {
    int value_;
    NoCopyConstructType(int value) : value_(value) {}
    NoCopyConstructType(const NoCopyConstructType&) = delete;
    NoCopyConstructType& operator=(const NoCopyConstructType&) = default;
};

static_assert(std::is_trivially_copy_assignable_v<NoCopyConstructType>);
static_assert(!std::is_copy_constructible_v<NoCopyConstructType>);

// Trivially constructible but not copy assignable.

struct NoCopyAssignType {
    int value_;
    NoCopyAssignType(int value) : value_(value) {}
    NoCopyAssignType(const NoCopyAssignType&) = default;
    NoCopyAssignType& operator=(const NoCopyAssignType&) = delete;
};

static_assert(std::is_trivially_copy_constructible_v<NoCopyAssignType>);
static_assert(!std::is_copy_assignable_v<NoCopyAssignType>);

// Trivially move assignable but not move constructible.

struct NoMoveConstructType {
    int value_;
    NoMoveConstructType() = default;
    explicit NoMoveConstructType(int value) : value_(value) {}
    NoMoveConstructType(const NoMoveConstructType&) = default;
    NoMoveConstructType(NoMoveConstructType&&) = delete;
    NoMoveConstructType& operator=(const NoMoveConstructType&) = default;
    NoMoveConstructType& operator=(NoMoveConstructType&&) = default;
};

static_assert(std::is_trivially_move_assignable_v<NoMoveConstructType>);
static_assert(!std::is_move_constructible_v<NoMoveConstructType>);

// Trivially move constructible but not move assignable.

struct NoMoveAssignType {
    int value_;
    NoMoveAssignType() = default;
    explicit NoMoveAssignType(int value) : value_(value) {}
    NoMoveAssignType(const NoMoveAssignType&) = default;
    NoMoveAssignType(NoMoveAssignType&&) = default;
    NoMoveAssignType& operator=(const NoMoveAssignType&) = default;
    NoMoveAssignType& operator=(NoMoveAssignType&&) = delete;
};

static_assert(std::is_trivially_move_constructible_v<NoMoveAssignType>);
static_assert(!std::is_move_assignable_v<NoMoveAssignType>);

// Constructible but not trivially constructible.

struct NonTrivialConstructType {
    int value_;
    NonTrivialConstructType() = default;
    ~NonTrivialConstructType() = default;
    NonTrivialConstructType(int value) : value_(value) {}
    NonTrivialConstructType(const NonTrivialConstructType&) = default;
    NonTrivialConstructType(NonTrivialConstructType&&) = default;
    NonTrivialConstructType& operator=(const NonTrivialConstructType&) = default;
    NonTrivialConstructType& operator=(NonTrivialConstructType&&) = default;
};

static_assert(std::is_constructible_v<NonTrivialConstructType, int>);
static_assert(!std::is_trivially_constructible_v<NonTrivialConstructType, int>);

// Default constructible but not trivially default constructible.

struct NonTrivialDefaultConstructType {
    int value_;
    NonTrivialDefaultConstructType() noexcept {}
    ~NonTrivialDefaultConstructType() = default;
    NonTrivialDefaultConstructType(const NonTrivialDefaultConstructType&) = default;
    NonTrivialDefaultConstructType(NonTrivialDefaultConstructType&&) = default;
    NonTrivialDefaultConstructType& operator=(const NonTrivialDefaultConstructType&) = default;
    NonTrivialDefaultConstructType& operator=(NonTrivialDefaultConstructType&&) = default;
};

static_assert(std::is_default_constructible_v<NonTrivialDefaultConstructType>);
static_assert(!std::is_trivially_default_constructible_v<NonTrivialDefaultConstructType>);

// Destructible but not trivially destructible.

struct NonTrivialDestructType {
    int value_;
    NonTrivialDestructType() = default;
    ~NonTrivialDestructType() noexcept {}
    NonTrivialDestructType(const NonTrivialDestructType&) = default;
    NonTrivialDestructType(NonTrivialDestructType&&) = default;
    NonTrivialDestructType& operator=(const NonTrivialDestructType&) = default;
    NonTrivialDestructType& operator=(NonTrivialDestructType&&) = default;
};

static_assert(std::is_destructible_v<NonTrivialDestructType>);
static_assert(!std::is_trivially_destructible_v<NonTrivialDestructType>);

// Copy constructible but not trivially copy constructible.

struct NonTrivialCopyConstructType {
    int value_;
    NonTrivialCopyConstructType() = default;
    ~NonTrivialCopyConstructType() = default;
    NonTrivialCopyConstructType(const NonTrivialCopyConstructType&) {}
    NonTrivialCopyConstructType(NonTrivialCopyConstructType&&) = default;
    NonTrivialCopyConstructType& operator=(const NonTrivialCopyConstructType&) = default;
    NonTrivialCopyConstructType& operator=(NonTrivialCopyConstructType&&) = default;
};

static_assert(std::is_copy_constructible_v<NonTrivialCopyConstructType>);
static_assert(!std::is_trivially_copy_constructible_v<NonTrivialCopyConstructType>);

// Copy assignable but not trivially copy assignable.

struct NonTrivialCopyAssignType {
    int value_;
    NonTrivialCopyAssignType() = default;
    ~NonTrivialCopyAssignType() = default;
    NonTrivialCopyAssignType(const NonTrivialCopyAssignType&) = default;
    NonTrivialCopyAssignType(NonTrivialCopyAssignType&&) = default;
    NonTrivialCopyAssignType& operator=(const NonTrivialCopyAssignType&) { return *this; } // NOLINT
    NonTrivialCopyAssignType& operator=(NonTrivialCopyAssignType&&) = default;
};

static_assert(std::is_copy_assignable_v<NonTrivialCopyAssignType>);
static_assert(!std::is_trivially_copy_assignable_v<NonTrivialCopyAssignType>);

// Move constructible but not trivially move constructible.

struct NonTrivialMoveConstructType {
    int value_;
    NonTrivialMoveConstructType() = default;
    ~NonTrivialMoveConstructType() = default;
    NonTrivialMoveConstructType(const NonTrivialMoveConstructType&) = default;
    NonTrivialMoveConstructType(NonTrivialMoveConstructType&&) noexcept {}
    NonTrivialMoveConstructType& operator=(const NonTrivialMoveConstructType&) = default;
    NonTrivialMoveConstructType& operator=(NonTrivialMoveConstructType&&) = default;
};

static_assert(std::is_move_constructible_v<NonTrivialMoveConstructType>);
static_assert(!std::is_trivially_move_constructible_v<NonTrivialMoveConstructType>);

// Move assignable but not trivially move assignable.

struct NonTrivialMoveAssignType {
    int value_;
    NonTrivialMoveAssignType() = default;
    ~NonTrivialMoveAssignType() = default;
    NonTrivialMoveAssignType(const NonTrivialMoveAssignType&) = default;
    NonTrivialMoveAssignType(NonTrivialMoveAssignType&&) = default;
    NonTrivialMoveAssignType& operator=(const NonTrivialMoveAssignType&) = default;
    NonTrivialMoveAssignType& operator=(NonTrivialMoveAssignType&&) noexcept { return *this; } // NOLINT
};

static_assert(std::is_move_assignable_v<NonTrivialMoveAssignType>);
static_assert(!std::is_trivially_move_assignable_v<NonTrivialMoveAssignType>);

// Trivially copyable but not default constructible.

struct NoDefaultConstructType {
    int value_;
    NoDefaultConstructType() = delete;
    ~NoDefaultConstructType() = default;
    explicit NoDefaultConstructType(int value) : value_(value) {}
    NoDefaultConstructType(const NoDefaultConstructType&) = default;
    NoDefaultConstructType(NoDefaultConstructType&&) = default;
    NoDefaultConstructType& operator=(const NoDefaultConstructType&) = default;
    NoDefaultConstructType& operator=(NoDefaultConstructType&&) = default;
};

static_assert(std::is_trivially_copyable_v<NoDefaultConstructType>);
static_assert(!std::is_default_constructible_v<NoDefaultConstructType>);

// Constructible but not nothrow constructible.

struct ThrowConstructType {
    int value_;
    ThrowConstructType() noexcept = default;
    ThrowConstructType(int) noexcept(false) {}
    ~ThrowConstructType() noexcept = default;
    ThrowConstructType(const ThrowConstructType&) noexcept = default;
    ThrowConstructType(ThrowConstructType&&) noexcept = default;
    ThrowConstructType& operator=(const ThrowConstructType&) noexcept = default;
    ThrowConstructType& operator=(ThrowConstructType&&) noexcept = default;
};

static_assert(std::is_constructible_v<ThrowConstructType, int>);
static_assert(!std::is_nothrow_constructible_v<ThrowConstructType, int>);

// Default constructible but not nothrow default constructible.

struct ThrowDefaultConstructType {
    int value_ {};
    constexpr ThrowDefaultConstructType() noexcept(false) {}
    ~ThrowDefaultConstructType() noexcept = default;
    ThrowDefaultConstructType(const ThrowDefaultConstructType&) noexcept = default;
    ThrowDefaultConstructType(ThrowDefaultConstructType&&) noexcept = default;
    ThrowDefaultConstructType& operator=(const ThrowDefaultConstructType&) noexcept = default;
    ThrowDefaultConstructType& operator=(ThrowDefaultConstructType&&) noexcept = default;
};

static_assert(std::is_default_constructible_v<ThrowDefaultConstructType>);
static_assert(!std::is_nothrow_default_constructible_v<ThrowDefaultConstructType>);

// Value type with maximal exception surface.

struct ThrowAllValueType {
    int value_;
    ThrowAllValueType() noexcept(false) {}
    ~ThrowAllValueType() noexcept(false) {}
    ThrowAllValueType(int) noexcept(false) {}
    ThrowAllValueType(const ThrowAllValueType&) noexcept(false) {}
    ThrowAllValueType(ThrowAllValueType&&) noexcept(false) {}
    ThrowAllValueType& operator=(const ThrowAllValueType&) noexcept(false) { return *this; } // NOLINT
    ThrowAllValueType& operator=(ThrowAllValueType&&) noexcept(false) { return *this; }
};

static_assert(!std::is_nothrow_destructible_v<ThrowAllValueType>);
static_assert(!std::is_nothrow_constructible_v<ThrowAllValueType, int>);
static_assert(!std::is_nothrow_default_constructible_v<ThrowAllValueType>);
static_assert(!std::is_nothrow_copy_constructible_v<ThrowAllValueType>);
static_assert(!std::is_nothrow_move_constructible_v<ThrowAllValueType>);
static_assert(!std::is_nothrow_copy_assignable_v<ThrowAllValueType>);
static_assert(!std::is_nothrow_move_assignable_v<ThrowAllValueType>);

// std::is_nothrow_constructible requires nothrow destructibility on some imple-
// mentations (LWG 2116).

struct ThrowAllButDestructValueType {
    int value_;
    ThrowAllButDestructValueType() noexcept(false) {}
    ~ThrowAllButDestructValueType() noexcept {}
    ThrowAllButDestructValueType(int) noexcept(false) {}
    ThrowAllButDestructValueType(const ThrowAllButDestructValueType&) noexcept(false) {}
    ThrowAllButDestructValueType(ThrowAllButDestructValueType&&) noexcept(false) {} // NOLINT
    ThrowAllButDestructValueType& operator=(const ThrowAllButDestructValueType&) noexcept(false) { return *this; } // NOLINT
    ThrowAllButDestructValueType& operator=(ThrowAllButDestructValueType&&) noexcept(false) { return *this; }
};

static_assert(std::is_nothrow_destructible_v<ThrowAllButDestructValueType>);
static_assert(!std::is_nothrow_constructible_v<ThrowAllButDestructValueType, int>);
static_assert(!std::is_nothrow_default_constructible_v<ThrowAllButDestructValueType>);
static_assert(!std::is_nothrow_copy_constructible_v<ThrowAllButDestructValueType>);
static_assert(!std::is_nothrow_move_constructible_v<ThrowAllButDestructValueType>);
static_assert(!std::is_nothrow_copy_assignable_v<ThrowAllButDestructValueType>);
static_assert(!std::is_nothrow_move_assignable_v<ThrowAllButDestructValueType>);

// Error type with maximal exception surface (modulo P1286R2).

struct ThrowAllErrorType {
    int value_;
    ThrowAllErrorType() noexcept(false) = default;
    ~ThrowAllErrorType() noexcept = default;
    ThrowAllErrorType(int) noexcept(false) {}
    ThrowAllErrorType(const ThrowAllErrorType&) noexcept = default;
    ThrowAllErrorType(ThrowAllErrorType&&) noexcept = default;
    ThrowAllErrorType& operator=(const ThrowAllErrorType&) noexcept(false) = default;
    ThrowAllErrorType& operator=(ThrowAllErrorType&&) noexcept(false) = default;
};

static_assert(std::is_trivial_v<ThrowAllErrorType>);
static_assert(std::is_trivially_copyable_v<ThrowAllErrorType>);
static_assert(std::is_trivially_destructible_v<ThrowAllErrorType>);
static_assert(std::is_trivially_copy_constructible_v<ThrowAllErrorType>);
static_assert(std::is_trivially_move_constructible_v<ThrowAllErrorType>);

static_assert(std::is_nothrow_destructible_v<ThrowAllErrorType>);
static_assert(std::is_nothrow_copy_constructible_v<ThrowAllErrorType>);
static_assert(std::is_nothrow_move_constructible_v<ThrowAllErrorType>);

static_assert(!std::is_nothrow_constructible_v<ThrowAllErrorType, int>);
static_assert(!std::is_nothrow_copy_assignable_v<ThrowAllErrorType>);
static_assert(!std::is_nothrow_move_assignable_v<ThrowAllErrorType>);

// P1286R2 permits an explicitly defaulted function to have an explicit noexcept
// specifier without losing triviality. The above default constructor should th-
// erefore be trivial and (potentially) throwing. Unfortunately, GCC erroneously
// reports the constructor as nothrow.

// static_assert(!std::is_nothrow_default_constructible_v<ThrowAllErrorType>);

// Destructible but not nothrow destructible.

struct ThrowDestructType {
    int value_;
    ThrowDestructType() noexcept = default;
    ~ThrowDestructType() noexcept(false) {}
    ThrowDestructType(const ThrowDestructType&) noexcept = default;
    ThrowDestructType(ThrowDestructType&&) noexcept = default;
    ThrowDestructType& operator=(const ThrowDestructType&) noexcept = default;
    ThrowDestructType& operator=(ThrowDestructType&&) noexcept = default;
};

static_assert(std::is_destructible_v<ThrowDestructType>);
static_assert(!std::is_nothrow_destructible_v<ThrowDestructType>);

// Copy constructible but not nothrow copy constructible.

struct ThrowCopyConstructType {
    int value_;
    ThrowCopyConstructType() noexcept = default;
    ~ThrowCopyConstructType() noexcept = default;
    ThrowCopyConstructType(const ThrowCopyConstructType&) noexcept(false) {}
    ThrowCopyConstructType(ThrowCopyConstructType&&) noexcept = default;
    ThrowCopyConstructType& operator=(const ThrowCopyConstructType&) noexcept = default;
    ThrowCopyConstructType& operator=(ThrowCopyConstructType&&) noexcept = default;
};

static_assert(std::is_copy_constructible_v<ThrowCopyConstructType>);
static_assert(!std::is_nothrow_copy_constructible_v<ThrowCopyConstructType>);

// Move constructible but not nothrow move constructible.

struct ThrowMoveConstructType {
    int value_;
    ThrowMoveConstructType() noexcept = default;
    ~ThrowMoveConstructType() noexcept = default;
    ThrowMoveConstructType(const ThrowMoveConstructType&) noexcept = default;
    ThrowMoveConstructType(ThrowMoveConstructType&&) noexcept(false) {} // NOLINT
    ThrowMoveConstructType& operator=(const ThrowMoveConstructType&) noexcept = default;
    ThrowMoveConstructType& operator=(ThrowMoveConstructType&&) noexcept = default;
};

static_assert(std::is_move_constructible_v<ThrowMoveConstructType>);
static_assert(!std::is_nothrow_move_constructible_v<ThrowMoveConstructType>);

// Copy assignable but not nothrow copy assignable.

struct ThrowCopyAssignType {
    int value_;
    ThrowCopyAssignType() noexcept = default;
    ~ThrowCopyAssignType() noexcept = default;
    ThrowCopyAssignType(const ThrowCopyAssignType&) noexcept = default;
    ThrowCopyAssignType(ThrowCopyAssignType&&) noexcept = default;
    ThrowCopyAssignType& operator=(const ThrowCopyAssignType&) noexcept(false) { return *this; } // NOLINT
    ThrowCopyAssignType& operator=(ThrowCopyAssignType&&) noexcept = default;
};

static_assert(std::is_copy_assignable_v<ThrowCopyAssignType>);
static_assert(!std::is_nothrow_copy_assignable_v<ThrowCopyAssignType>);

// Move assignable but not nothrow move assignable.

struct ThrowMoveAssignType {
    int value_;
    ThrowMoveAssignType() noexcept = default;
    ~ThrowMoveAssignType() noexcept = default;
    ThrowMoveAssignType(const ThrowMoveAssignType&) noexcept = default;
    ThrowMoveAssignType(ThrowMoveAssignType&&) noexcept = default;
    ThrowMoveAssignType& operator=(const ThrowMoveAssignType&) noexcept = default;
    ThrowMoveAssignType& operator=(ThrowMoveAssignType&&) noexcept(false) { return *this; }
};

static_assert(std::is_move_assignable_v<ThrowMoveAssignType>);
static_assert(!std::is_nothrow_move_assignable_v<ThrowMoveAssignType>);

// Trivially copyable but non-standard layout.

struct NonStandardLayoutType {
    public: int public_;
    private: int private_;
    [[nodiscard]] int f() const { return public_ + private_; } /* silence compiler */
};

static_assert(std::is_trivially_copyable_v<NonStandardLayoutType>);
static_assert(!std::is_standard_layout_v<NonStandardLayoutType>);

// Distinguish throwing from non-throwing constructor.

struct ConditionalThrowType {
    int value_;
    explicit ConditionalThrowType(int) noexcept : value_{0} {}
    constexpr explicit ConditionalThrowType(double) : value_{1} {}
};

static_assert(std::is_nothrow_constructible_v<ConditionalThrowType, int>);
static_assert(!std::is_nothrow_constructible_v<ConditionalThrowType, double>);

// Distinguish throwing from non-throwing default constructor.

struct DefaultThrowType {
    int value_;
    DefaultThrowType() : value_ {} {}
};

static_assert(std::is_default_constructible_v<DefaultThrowType>);
static_assert(!std::is_nothrow_default_constructible_v<DefaultThrowType>);

// Track the value category of forwarded arguments. The forwarding constructors
// are constrained to the cvref-unqualified type to prevent hijacking the copy
// and move constructors (so the class remains trivially copyable).

[[nodiscard]] constexpr ForwardCategory forward_category(int&) noexcept {
    return ForwardCategory::LValue;
}

[[nodiscard]] constexpr ForwardCategory forward_category(const int&) noexcept {
    return ForwardCategory::ConstLValue;
}

[[nodiscard]] constexpr ForwardCategory forward_category(int&&) noexcept { // NOLINT
    return ForwardCategory::RValue;
}

[[nodiscard]] constexpr ForwardCategory forward_category(const int&&) noexcept {
    return ForwardCategory::ConstRValue;
}

struct ForwardProbeType {

    ForwardCategory fst_ { ForwardCategory::None };
    ForwardCategory snd_ { ForwardCategory::None };

    template <typename A>
    requires std::same_as<int, std::remove_cvref_t<A>>
    constexpr explicit ForwardProbeType(A&& fst) noexcept :
        fst_ { forward_category(std::forward<A>(fst)) },
        snd_ { ForwardCategory::None } {}

    template <typename A, typename B>
    requires std::same_as<int, std::remove_cvref_t<A>> &&
             std::same_as<int, std::remove_cvref_t<B>>
    constexpr ForwardProbeType(A&& fst, B&& snd) noexcept :
        fst_ { forward_category(std::forward<A>(fst)) },
        snd_ { forward_category(std::forward<B>(snd)) } {}

};

static_assert(std::is_trivially_copyable_v<ForwardProbeType>);
static_assert(std::is_trivially_copy_constructible_v<ForwardProbeType>);
static_assert(std::is_trivially_move_constructible_v<ForwardProbeType>);

template <typename... Es>
struct UniverseT {

    using RankedTypes = type_pack_t<Es...>;

    template <typename T>
    requires pack_member_v<std::remove_cvref_t<T>, RankedTypes>
    static constexpr std::size_t rank = pack_index_v<std::remove_cvref_t<T>, RankedTypes>;

};

struct UniverseI : UniverseT<
    TrivialType,
    NoCopyConstructType,
    NoCopyAssignType,
    NoMoveConstructType,
    NoMoveAssignType,
    NonTrivialConstructType,
    NonTrivialDestructType,
    NonTrivialCopyConstructType,
    NonTrivialCopyAssignType,
    NonTrivialMoveConstructType,
    NonTrivialMoveAssignType,
    NoDefaultConstructType,
    NonStandardLayoutType,
    ConditionalThrowType,
    ForwardProbeType
> {};

} // namespace varerr::tests::universe

#endif // VARERR_TESTS_UNIVERSE_HPP
