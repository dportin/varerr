#ifndef VARERR_UTILITIES_HPP
#define VARERR_UTILITIES_HPP

#include <type_traits>

namespace varerr {

// Transfer const qualifier from one possibly ref-qualified type to another.

template <typename From, typename To>
using transfer_const_t =
    std::conditional_t<std::is_const_v<std::remove_reference_t<From>>, const To, To>;

} // namespace varerr

#endif // VARERR_UTILITIES_HPP
