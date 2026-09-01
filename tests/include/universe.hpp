#ifndef VARERR_TESTS_UNIVERSE
#define VARERR_TESTS_UNIVERSE

#include <bit>
#include <cassert>
#include <compare>
#include <cstddef>
#include <limits>
#include <utility>

namespace varerr::tests::universe {

// Universe of trivially storable types parameterized by rank.

template <std::size_t N>
struct E {

    std::size_t value_;

    [[nodiscard]] constexpr std::size_t value() const noexcept {
        return this->value_;
    }

};

template <std::size_t N, std::size_t M>
[[nodiscard]] constexpr bool operator==(const E<N>& lhs, const E<M>& rhs) noexcept {
    return N == M && lhs.value_ == rhs.value_;
}

template <std::size_t N, std::size_t M>
[[nodiscard]] constexpr std::strong_ordering operator<=>(const E<N>& lhs, const E<M>& rhs) noexcept {
    return N == M ? lhs.value_ <=> rhs.value_ : N <=> M;
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

// Heterogeneous universe of trivially storable types parameterized by payload
// size and alignment (which must be a power of two).

template <std::size_t N, std::size_t A>
struct H;

template <std::size_t A>
struct alignas(A) H<0, A> {

    static_assert(std::has_single_bit(A));

    [[nodiscard]] friend constexpr bool operator==(const H&, const H&) noexcept = default;

};

template <std::size_t N, std::size_t A>
struct H {

    static_assert(std::has_single_bit(A));

    alignas(A) unsigned char value_[N];

    [[nodiscard]] friend constexpr bool operator==(const H&, const H&) noexcept = default;

};

[[nodiscard]] constexpr std::size_t isqrt(std::size_t z) noexcept {

    // Compute the integer square root of z using binary search.

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

[[nodiscard]] constexpr std::size_t ilog2(std::size_t z) noexcept {

    // Compute the integer logarithm of z.

    assert(z > 0);
    assert(std::has_single_bit(z));

    return static_cast<std::size_t>(std::bit_width(z)) - 1;

}

[[nodiscard]] constexpr std::size_t szudzik_pair(std::size_t n, std::size_t m) noexcept {

    return n < m ? m * m + n : n * n + n + m;

}

[[nodiscard]] constexpr std::pair<std::size_t, std::size_t> szudzik_unpair(std::size_t z) noexcept {

    const std::size_t root = isqrt(z);
    const std::size_t disc = z - root * root;

    return disc < root ? std::pair { disc, root } : std::pair { root, disc - root };

}

struct UniverseH {

    template <typename T>
    struct rank_trait;

    template <std::size_t N, std::size_t A>
    struct rank_trait<H<N, A>> {
        static constexpr std::size_t rank_ = szudzik_pair(N, ilog2(A));
    };

    template <typename T>
    requires requires { rank_trait<T>::rank_; }
    static constexpr std::size_t rank = rank_trait<T>::rank_;

    template <std::size_t Z>
    struct unrank_trait {
        static constexpr auto unpair = szudzik_unpair(Z);
        static_assert(unpair.second < std::numeric_limits<std::size_t>::digits);
        using type = H<unpair.first, std::size_t {1} << unpair.second>;
    };

    template <std::size_t Z>
    using unrank = typename unrank_trait<Z>::type;

};

template <typename M, typename E>
inline constexpr std::size_t unrank_v = M::template unrank<E>;

inline constexpr std::size_t x = UniverseH::template rank<H<0,1>>;

static_assert(std::same_as<
    UniverseH::template unrank<UniverseH::template rank<H<1,1>>>,
    H<1,1>
>);

static_assert(std::same_as<
    UniverseH::template unrank<UniverseH::template rank<H<2,2>>>,
    H<2,2>
>);


} // namespace varerr::tests::universe

#endif // VARERR_TESTS_UNIVERSE
