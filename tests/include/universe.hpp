#ifndef VARERR_TESTS_UNIVERSE
#define VARERR_TESTS_UNIVERSE

#include <cassert>
#include <climits>
#include <compare>
#include <cstddef>
#include <span>
#include <tuple>
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

    std::size_t value_;

    constexpr explicit E(std::size_t value) noexcept : value_{value} {}

    [[nodiscard]] constexpr std::size_t value() const noexcept {
        return this->value_;
    }

};

template <std::size_t N, std::size_t M>
[[nodiscard]] constexpr bool operator==(const E<N>& lhs, const E<M>& rhs) noexcept {
    return std::tie(N, lhs.value()) == std::tie(M, rhs.value());
}

template <std::size_t N, std::size_t M>
[[nodiscard]] constexpr std::strong_ordering operator<=>(const E<N>& lhs, const E<M>& rhs) noexcept {
    return std::tie(N, lhs.value()) <=> std::tie(M, rhs.value());
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

// Heterogeneous universe of trivially storable types parameterized by payload
// size and log-alignment.

inline constexpr std::size_t kMaxLogAlign = 13; /* 2^13 = 8192 */

template <std::size_t N, std::size_t A>
struct H;

template <std::size_t A>
struct alignas((std::size_t {1} << A)) H<0, A> {

    static_assert(A <= kMaxLogAlign, "A must not exceed kMaxLogAlign");

    constexpr explicit H(std::size_t) noexcept {}

    [[nodiscard]] constexpr std::span<const unsigned char> bytes() const noexcept {
        return {};
    }

};

template <std::size_t N, std::size_t A>
struct H {

    static_assert(A <= kMaxLogAlign, "A must not exceed kMaxLogAlign");

    alignas(std::size_t {1} << A) unsigned char value_[N]; // NOLINT

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
[[nodiscard]] constexpr bool operator==(const H<N, A>& lhs, const H<M, A>& rhs) noexcept {
    return std::tie(N, A, lhs.bytes()) == std::tie(M, B, rhs.bytes());
}

template <std::size_t N, std::size_t A, std::size_t M, std::size_t B>
[[nodiscard]] constexpr std::strong_ordering operator<=>(const H<N, A>& lhs, const H<M, A>& rhs) noexcept {
    return std::tie(N, A, lhs.bytes()) <=> std::tie(M, B, rhs.bytes());
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

// Assorted types designed to break specific invariants.

// Trivially copy assignable but not copy constructible.

struct NoCopyConstructType {
    int value_;
    NoCopyConstructType(const NoCopyConstructType&) = delete;
    NoCopyConstructType& operator=(const NoCopyConstructType&) = default;
};

static_assert(std::is_trivially_copy_assignable_v<NoCopyConstructType>);
static_assert(!std::is_copy_constructible_v<NoCopyConstructType>);

// Trivially constructible but not copy assignable.

struct NoCopyAssignType {
    int value_;
    NoCopyAssignType(const NoCopyAssignType&) = default;
    NoCopyAssignType& operator=(const NoCopyAssignType&) = delete;
};

static_assert(std::is_trivially_copy_constructible_v<NoCopyAssignType>);
static_assert(!std::is_copy_assignable_v<NoCopyAssignType>);

// Trivially move assignable but not move constructible.

struct NoMoveConstructType {
    int value_;
    NoMoveConstructType(NoMoveConstructType&&) = delete;
    NoMoveConstructType& operator=(NoMoveConstructType&&) noexcept = default;
};

static_assert(std::is_trivially_move_assignable_v<NoMoveConstructType>);
static_assert(!std::is_move_constructible_v<NoMoveConstructType>);

// Trivially move constructible but not move assignable.

struct NoMoveAssignType {
    int value_;
    NoMoveAssignType(NoMoveAssignType&&) = default;
    NoMoveAssignType& operator=(NoMoveAssignType&&) noexcept = delete;
};

static_assert(std::is_trivially_move_constructible_v<NoMoveAssignType>);
static_assert(!std::is_move_assignable_v<NoMoveAssignType>);

// Constructible but not trivially constructible.

struct NonTrivialConstructType {
    int value_;
    NonTrivialConstructType(int value) : value_(value) {}
};

static_assert(std::is_constructible_v<NonTrivialConstructType, int>);
static_assert(!std::is_trivially_constructible_v<NonTrivialConstructType, int>);

// Destructible but not trivially destructible.

struct NonTrivialDestructType {
    int value_;
    ~NonTrivialDestructType() {}
};

static_assert(std::is_destructible_v<NonTrivialDestructType>);
static_assert(!std::is_trivially_destructible_v<NonTrivialDestructType>);

// Copy constructible but not trivially copy constructible.

struct NonTrivialCopyConstructType {
    int value_;
    NonTrivialCopyConstructType(const NonTrivialCopyConstructType&) {}
};

static_assert(std::is_copy_constructible_v<NonTrivialCopyConstructType>);
static_assert(!std::is_trivially_copy_constructible_v<NonTrivialCopyConstructType>);

// Copy assignable but not trivially copy assignable.

struct NonTrivialCopyAssignType {
    int value_;
    NonTrivialCopyAssignType& operator=(const NonTrivialCopyAssignType&) { return *this; } // NOLINT
};

static_assert(std::is_copy_assignable_v<NonTrivialCopyAssignType>);
static_assert(!std::is_trivially_copy_assignable_v<NonTrivialCopyAssignType>);

// Move constructible but not trivially move constructible.

struct NonTrivialMoveConstructType {
    int value_;
    NonTrivialMoveConstructType(NonTrivialMoveConstructType&&) noexcept {}
};

static_assert(std::is_move_constructible_v<NonTrivialMoveConstructType>);
static_assert(!std::is_trivially_move_constructible_v<NonTrivialMoveConstructType>);

// Move assignable but not trivially move assignable.

struct NonTrivialMoveAssignType {
    int value_;
    NonTrivialMoveAssignType& operator=(NonTrivialMoveAssignType&&) { return *this; } // NOLINT
};

static_assert(std::is_move_assignable_v<NonTrivialMoveAssignType>);
static_assert(!std::is_trivially_move_assignable_v<NonTrivialMoveAssignType>);

// Trivially copyable but not default constructible.

struct NoDefaultConstructType {
    int value_;
    NoDefaultConstructType() = delete;
};

static_assert(std::is_trivially_copyable_v<NoDefaultConstructType>);
static_assert(!std::is_default_constructible_v<NoDefaultConstructType>);

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

// Observe the value category of forwarded arguments. The probe must be trivial-
// ly storable and thus trivially copyable (so the forwarding category is track-
// ed through converting constructors rather than copy and move constructors).

enum class ForwardCategory : unsigned char {
    None,
    LValue,
    ConstLValue,
    RValue,
    ConstRValue
};

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

} // namespace varerr::tests::universe

#endif // VARERR_TESTS_UNIVERSE
