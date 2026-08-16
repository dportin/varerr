#include <varerr/varset.hpp>
#include <varerr/status.hpp>
#include <varerr/result.hpp>

#include <utility>

int main(void) {

    // Instantiate varset::detail::Storage

    const varerr::detail::Storage<int> v1 { std::in_place_index<0>, 42 };
    const varerr::detail::Storage<short, int> v2 { std::in_place_index<1>, 42 };

    return v1.head_ == v2.tail_.head_ && varerr::kStatusStub && varerr::kResultStub; // NOLINT

}
