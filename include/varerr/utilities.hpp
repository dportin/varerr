#ifndef VARERR_UTILITIES_HPP
#define VARERR_UTILITIES_HPP

#include <concepts>
#include <type_traits>

namespace varerr {

// Transfer const qualifier from one possible ref-qualified type to another.

template <typename From, typename To>
using transfer_const_t = std::conditional_t<std::is_const_v<std::remove_reference_t<From>>, const To, To>;

// Determine whether a type T is a non-volatile lvalue.

template <typename T>
concept IsNonVolatile = !std::is_volatile_v<std::remove_reference_t<T>>;

template <typename T>
concept IsNonVolatileLValueReference = std::is_lvalue_reference_v<T> && IsNonVolatile<T>;

// Determine whether every element of a parameter pack has the same type.

template <typename... Es>
struct is_uniform : std::true_type {};

template <typename E, typename... Es>
struct is_uniform<E, Es...> : std::bool_constant<(std::same_as<E, Es> && ...)> {};

template <typename... Es>
inline constexpr bool is_uniform_v = is_uniform<Es...>::value;

// Apply a metafunction F to the parameter pack carried by M.

namespace detail {

template <typename F, typename M>
struct pack_apply_impl {};

template <typename F, template <typename...> typename M, typename... Es>
requires requires { typename F::template bind<Es...>; }
struct pack_apply_impl<F, M<Es...>> {
    using bind_type = typename F::template bind<Es...>;
};

} // namespace detail

// Determine whether a metafunction F can be applied to the parameter pack car-
// ried by M.

template <typename F, typename M>
concept IsPackApplyBindWellFormed = requires {
    typename detail::pack_apply_impl<F, M>::bind_type;
};

template <typename F, typename M>
concept IsPackApplyTypeWellFormed = IsPackApplyBindWellFormed<F, M> && requires {
    typename detail::pack_apply_impl<F, M>::bind_type::type;
};

template <typename F, typename M>
concept IsPackApplyValueWellFormed = IsPackApplyBindWellFormed<F, M> && requires {
    detail::pack_apply_impl<F, M>::bind_type::value;
};

// Apply a metafunction F to the parameter pack carried by M.

template <typename F, typename M>
requires IsPackApplyBindWellFormed<F, M>
struct pack_apply : detail::pack_apply_impl<F, M> {};

template <typename F, typename M>
requires IsPackApplyTypeWellFormed<F, M>
using pack_apply_t = pack_apply<F, M>::bind_type::type;

template <typename F, typename M>
requires IsPackApplyValueWellFormed<F, M>
inline constexpr auto pack_apply_v = pack_apply<F, M>::bind_type::value;

// The identity adapter passes a metafunction F to pack_apply.

template <template <typename...> typename F, typename... Args>
struct bind_meta_adapter {
    template <typename... Es>
    using bind = F<Args..., Es...>;
};

template <template <typename...> typename F, typename... Args>
struct bind_meta_back_adapter {
    template <typename... Es>
    using bind = F<Es..., Args...>;
};

// The lift combinator lifts a parameterized type to a metafunction.

template <typename F>
struct bind_lift {
    template <typename... Es>
    using bind = std::type_identity<typename F::template bind<Es...>>;
};

// The lift adapter passes a metafunction F to pack_apply.

template <template <typename...> typename F, typename... Args>
using bind_lift_adapter = bind_lift<bind_meta_adapter<F, Args...>>;

template <template <typename...> typename F, typename... Args>
using bind_lift_back_adapter = bind_lift<bind_meta_back_adapter<F, Args...>>;

} // namespace varerr

#endif // VARERR_UTILITIES_HPP
