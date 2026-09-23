#ifndef VARERR_TESTS_FUNCTOR_HPP
#define VARERR_TESTS_FUNCTOR_HPP

#include "utilities.hpp"

namespace varerr::tests::functor {

// Differentiated by value category and constness of the parameter. The return
// type and functor qualifiers are fixed.

template <typename T>
struct FunctorOnConstValueFromValueToValue {
    [[maybe_unused]] constexpr T operator()(T value) const { return value; }
};

template <typename T>
struct FunctorOnConstValueFromConstValueToValue {
    [[maybe_unused]] constexpr T operator()(const T value) const { return value; }
};

template <typename T>
struct FunctorOnConstValueFromLValueRefToValue {
    [[maybe_unused]] constexpr T operator()(T& value) const { return value; }
};

template <typename T>
struct FunctorOnConstValueFromConstLValueRefToValue {
    [[maybe_unused]] constexpr T operator()(const T& value) const { return value; }
};

template <typename T>
struct FunctorOnConstValueFromRValueRefToValue {
    [[maybe_unused]] constexpr T operator()(T&& value) const { return std::move(value); }
};

template <typename T>
struct FunctorOnConstValueFromConstRValueRefToValue {
    [[maybe_unused]] constexpr T operator()(const T&& value) const { return value; }
};

// Differentiated by value category and constness of the return type. The funct-
// or qualifiers are fixed while the parameters have the least-constrained type
// from which the return type can be formed.

template <typename T>
struct FunctorOnConstValueFromLeastConstrainedToValue {
    [[maybe_unused]] constexpr T operator()(const T& value) const { return value; }
};

template <typename T>
struct FunctorOnConstValueFromLeastConstrainedToConstValue {
    [[maybe_unused]] constexpr const T operator()(const T& value) const { return value; }
};

template <typename T>
struct FunctorOnConstValueFromLeastConstrainedToLValueRef {
    [[maybe_unused]] constexpr T& operator()(T& value) const { return value; }
};

template <typename T>
struct FunctorOnConstValueFromLeastConstrainedToConstLValueRef {
    [[maybe_unused]] constexpr const T& operator()(const T& value) const { return value; }
};

template <typename T>
struct FunctorOnConstValueFromLeastConstrainedToRValueRef {
    [[maybe_unused]] constexpr T&& operator()(T&& value) const { return std::move(value); }
};

template <typename T>
struct FunctorOnConstValueFromLeastConstrainedToConstRValueRef {
    [[maybe_unused]] constexpr const T&& operator()(const T&& value) const { return std::move(value); }
};

// Differentiated by value category and constness of the functor. The parameter
// and return type are fixed.

template <typename T>
struct FunctorOnValueFromConstLValueRefToValue {
    [[maybe_unused]] constexpr T operator()(const T& value) { return value; }
};

// template <typename T>
// struct FunctorOnConstValueFromConstLValueRefToValue {
//     [[maybe_unused]] constexpr T operator()(const T& value) const { return value; }
// };

template <typename T>
struct FunctorOnLValueRefFromConstLValueRefToValue {
    [[maybe_unused]] constexpr T operator()(const T& value) & { return value; }
};

template <typename T>
struct FunctorOnConstLValueRefFromConstLValueRefToValue {
    [[maybe_unused]] constexpr T operator()(const T& value) const & { return value; }
};

template <typename T>
struct FunctorOnRValueRefFromConstLValueRefToValue {
    [[maybe_unused]] constexpr T operator()(const T& value) && { return value; }
};

template <typename T>
struct FunctorOnConstRValueRefFromConstLValueRefToValue {
    [[maybe_unused]] constexpr T operator()(const T& value) const && { return value; }
};

// Specialize test functors to void.

struct FunctorOnSelfFromForwardToVoid {
    template <typename Self, typename R>
    [[maybe_unused]] constexpr void operator()(this Self&&, R&&) {}
};

struct FunctorOnSelfFromVoidToVoid {
    template <typename Self>
    [[maybe_unused]] constexpr void operator()(this Self&&) {}
};

template <typename T>
struct FunctorOnSelfFromVoidToValue {
    template <typename Self>
    [[maybe_unused]] constexpr T operator()(this Self&&) { return T {}; }
};

// Forward the value category and implicit object parameter.

template <typename T>
struct FunctorOnSelfFromForwardToValue {
    template <typename Self, typename R>
    [[maybe_unused]] constexpr T operator()(this Self&&, R&& value) { return std::forward<R>(value); }
};

template <typename T>
struct FunctorOnSelfFromForwardToForward {
    template <typename Self, typename R>
    [[maybe_unused]] constexpr decltype(auto) operator()(this Self&&, R&& value) { return std::forward<R>(value); }
};

// Differentiate by value category and constness of the parameter while keeping
// the return value fixed.

template <typename T>
struct FunctorOnSelfFromValueToValue {
    template <typename Self>
    [[maybe_unused]] constexpr T operator()(this Self&&, T value) { return value; }
};

template <typename T>
struct FunctorOnSelfFromConstValueToValue {
    template <typename Self>
    [[maybe_unused]] constexpr T operator()(this Self&&, const T value) { return value; }
};

template <typename T>
struct FunctorOnSelfFromLValueRefToValue {
    template <typename Self>
    [[maybe_unused]] constexpr T operator()(this Self&&, T& value) { return value; }
};

template <typename T>
struct FunctorOnSelfFromConstLValueRefToValue {
    template <typename Self>
    [[maybe_unused]] constexpr T operator()(this Self&&, const T& value) { return value; }
};

template <typename T>
struct FunctorOnSelfFromRValueRefToValue {
    template <typename Self>
    [[maybe_unused]] constexpr T operator()(this Self&&, T&& value) { return std::move(value); }
};

template <typename T>
struct FunctorOnSelfFromConstRValueRefToValue {
    template <typename Self>
    [[maybe_unused]] constexpr T operator()(this Self&&, const T&& value) { return std::move(value); }
};

// Differentiated by the declared return type (independent of the parameter).

template <typename R>
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
inline std::remove_cvref_t<R> declared {};

template <typename R>
struct FunctorOnSelfFromForwardToDeclared {
    template <typename Self, typename T>
    [[maybe_unused]] constexpr R operator()(this Self&&, T&&) {

        static_assert(std::is_default_constructible_v<std::remove_cvref_t<R>>,
            "FunctorOnSelfFromForwardToDeclared: decayed return type must be default constructible");

        if constexpr (std::is_reference_v<R>) {
            return static_cast<R>(declared<R>);
        } else {
            return std::remove_cv_t<R> {};
        }

    }
};

// Report the value category of the forwarded and implicit object parameters.

struct FunctorProbeResult {
    ForwardCategory self_;
    ForwardCategory param_;
};

struct FunctorProbeParameter {

    template <typename Self, typename T>
    [[maybe_unused]] constexpr FunctorProbeResult operator()(this Self&&, T&&) noexcept {
        return {
            .self_ = FunctorProbeParameter::forward_category<Self>(),
            .param_ = FunctorProbeParameter::forward_category<T>()
        };
    }

    template <typename Self>
    [[maybe_unused]] constexpr FunctorProbeResult operator()(this Self&&) noexcept {
        return {
            .self_ = FunctorProbeParameter::forward_category<Self>(),
            .param_ = ForwardCategory::None
        };
    }

    template <typename T>
    static constexpr ForwardCategory forward_category() {

        constexpr ForwardCategory table[2][2] = {
            { ForwardCategory::RValue, ForwardCategory::ConstRValue },
            { ForwardCategory::LValue, ForwardCategory::ConstLValue }
        };

        constexpr bool is_lvalue = std::is_lvalue_reference_v<T>;
        constexpr bool is_const = std::is_const_v<std::remove_reference_t<T>>;

        return table[is_lvalue][is_const];

    }

};

} // varerr::tests::functor

#endif // VARERR_TESTS_FUNCTOR_HPP
