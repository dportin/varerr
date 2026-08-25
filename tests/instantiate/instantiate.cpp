#include <varerr/status.hpp>
#include <varerr/result.hpp>

#include <cstddef>
#include <utility>

#include "../include/universe.hpp"

using namespace varerr::tests::universe;

int main(void) {

    // Instantiate varerr::detail::Storage

    const varerr::detail::Storage<E<0>> storage0 { std::in_place_index<0>, std::size_t {42} };
    const varerr::detail::Storage<E<0>, E<1>> storage1 { std::in_place_index<1>, std::size_t {42} };
    const bool check_storage = storage0.head_.value() == storage1.tail_.head_.value();

    // Instantiate varerr::detail::Status

    const auto status0 = varerr::detail::Status<Universe, E<0>> { std::in_place_type<E<0>>, std::size_t {42} };
    const auto status1 = varerr::detail::Status<Universe, E<0>, E<1>> { std::in_place_type<E<1>>, std::size_t {42} };
    const bool check_status = status0.get_if<E<0>>()->value() == status1.get_if<E<1>>()->value();;

    // Instantiate varerr::Error

    const auto error0 = varerr::Error<E<0>> { E<0> {42} };
    const auto error1 = varerr::Error<E<1>> { E<1> {42} };
    const bool check_error = error0.unwrap().value() == error1.unwrap().value();

    // Instantiate varerr::detail::Result

    const auto result0 = varerr::Result<Universe, int, E<0>> { varerr::Error<E<0>> { E<0> {42} } };
    const auto result1 = varerr::Result<Universe, int, E<0>, E<1>> { varerr::Error<E<1>> { E<1> {42} } };
    const bool check_result = result0.error<E<0>>().value() == result1.error<E<1>>().value();

    return check_storage && check_status && check_error && check_result;

}
