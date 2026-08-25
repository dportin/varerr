#ifndef VARERR_TESTS_UNIVERSE
#define VARERR_TESTS_UNIVERSE

#include <compare>
#include <cstddef>

namespace varerr::tests::universe {

template <std::size_t N>
struct E {

    std::size_t value_;

    template <std::size_t>
    friend struct E;

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

} // namespace varerr::tests::universe

#endif // VARERR_TESTS_UNIVERSE
