#include <varerr/status.hpp>
#include <varerr/result.hpp>

#include <cstddef>
#include <utility>

namespace {

    template <std::size_t N>
    struct E {
        
        std::size_t value_;

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

} // namespace

int main(void) {

    // Instantiate varerr::detail::Storage

    const varerr::detail::Storage<E<0>> e0 { std::in_place_index<0>, static_cast<std::size_t>(42) };
    const varerr::detail::Storage<E<0>, E<1>> e1 { std::in_place_index<1>, static_cast<std::size_t>(42) };
    const bool check_storage = e0.head_ == e1.tail_.head_; // NOLINT 

    // Instantiate varerr::detail::Status

    const auto s0 = varerr::detail::StatusImpl<Universe, E<0>> { std::in_place_type<E<0>>, static_cast<std::size_t>(42) };
    const auto s1 = varerr::detail::StatusImpl<Universe, E<0>, E<1>> { std::in_place_type<E<1>>, static_cast<std::size_t>(42) };
    const bool check_status_impl = s0.holds<E<0>>() == s1.holds<E<1>>() && s1.holds<E<1>>() == s1.holds<E<0>>();

    // Instantiate varerr::Error

    const auto err0 = varerr::Error<E<0>> { E<0> { 42 } };
    const auto err1 = varerr::Error<E<1>> { E<1> { 42 } };
    const bool check_error = err0.unwrap() == err1.unwrap();

    // Instantiate varerr::detail::ResultImpl

    return check_storage && check_status_impl && check_error;

}
