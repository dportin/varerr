#ifndef VARERR_STORAGE_HPP
#define VARERR_STORAGE_HPP

#include <cstddef>
#include <concepts>
#include <memory>
#include <type_traits>
#include <utility>

// This file implements a trivially copyable storage class to back the variadic
// status class forming the unexpected branch of the main result class. The al-
// ternatives must be trivially copyable for the storage class to be implement-
// ed as a raw recursive union with no destructor accounting. The status class
// inherits trivial copyability, making it cheap and unconditionally noexcept.

namespace varerr {

template <typename E>
concept IsStorable =
    std::is_object_v<E> &&
    !std::is_array_v<E> &&
    std::same_as<E, std::remove_cv_t<E>>;

// Note that trivial copyability implies trivial destructibility, since every
// trivially copyable type must have a non-deleted trivial destructor. The re-
// quirement is repeated here for clarity.

template <typename E>
concept IsTriviallyStorable =
    IsStorable<E> &&
    std::is_trivially_copyable_v<E> &&
    std::is_trivially_destructible_v<E>;

namespace detail {

template <IsTriviallyStorable... Es>
union Storage {

    constexpr Storage() noexcept {}

};

template <IsTriviallyStorable E, IsTriviallyStorable... Es>
union Storage<E, Es...> {

    E head_;
    Storage<Es...> tail_;

    // PR1331R2 relaxes the requirement that every non-variant, non-static data
    // member and base class sub-object must be initialized in a constexpr con-
    // text (provided that the uninitialized values are not accessed).

    constexpr Storage() noexcept {}

    template <typename... Args>
    requires std::constructible_from<E, Args...>
    constexpr explicit Storage(std::in_place_index_t<0>, Args&&... args)
    noexcept(std::is_nothrow_constructible_v<E, Args...>) :
        head_(std::forward<Args>(args)...) {}

    template <std::size_t N, typename... Args>
    requires (N > 0) &&
             (N <= sizeof...(Es)) &&
             std::constructible_from<Storage<Es...>, std::in_place_index_t<N - 1>, Args...>
    constexpr explicit Storage(std::in_place_index_t<N>, Args&&... args)
    noexcept(std::is_nothrow_constructible_v<Storage<Es...>, std::in_place_index_t<N - 1>, Args...>) :
        tail_(std::in_place_index<N - 1>, std::forward<Args>(args)...) {}

};

// Determine whether a type is a storage type.

template <typename S>
struct is_storage : std::false_type {};

template <IsTriviallyStorable... Es>
struct is_storage<Storage<Es...>> : std::true_type {};

template <typename S>
inline constexpr bool is_storage_v = is_storage<S>::value;

template <typename S>
concept IsStorage = is_storage_v<std::remove_cvref_t<S>>;

// Determine the number of alternatives in a storage type.

template <typename S>
struct storage_size;

template <IsTriviallyStorable... Es>
struct storage_size<Storage<Es...>> : std::integral_constant<std::size_t, sizeof...(Es)> {};

template <IsStorage S>
inline constexpr std::size_t storage_size_v = storage_size<std::remove_cvref_t<S>>::value;

// Determine the type of the alternative at index N in a storage type.

template <std::size_t N, typename S>
struct storage_alternative;

template <IsTriviallyStorable E, IsTriviallyStorable... Es>
struct storage_alternative<0, Storage<E, Es...>> : std::type_identity<E> {};

template <std::size_t N, IsTriviallyStorable E, IsTriviallyStorable... Es>
struct storage_alternative<N, Storage<E, Es...>> : storage_alternative<N - 1, Storage<Es...>> {};

template <std::size_t N, IsStorage S>
requires (N < storage_size_v<S>)
using storage_alternative_t = typename storage_alternative<N, std::remove_cvref_t<S>>::type;

// Return a reference to the alternative at index N in a storage type. Reading
// the reference is undefined if it does not refer to the active alternative.

template <std::size_t N, IsStorage S>
requires (N < storage_size_v<S>)
[[nodiscard]] constexpr auto& storage_get(S& storage) noexcept {
    if constexpr (N == 0) {
        return storage.head_;
    } else {
        return storage_get<N - 1>(storage.tail_);
    }
}

// Activate each union on the path to the alternative at index N in a storage
// type, leaving the selected alternative as raw storage for the caller to con-
// struct in-place. The previously active alternative is discarded without cal-
// ling its destructor. This is well-defined because every alternative is triv-
// ially destructible by construction.

template <std::size_t N, IsStorage S>
requires (!std::is_const_v<S>) &&
         (N < storage_size_v<S>)
[[nodiscard]] constexpr auto* storage_emplace_impl(S& storage) noexcept {
    if constexpr (N == 0) {
        return std::addressof(storage.head_);
    } else {
        std::construct_at(std::addressof(storage.tail_));
        return storage_emplace_impl<N - 1>(storage.tail_);
    }
}

// Activate the alternative at index N in a storage type. Note that the pointer
// returned by std::construct_at is never null.

template <std::size_t N, IsStorage S, typename... Args>
requires (!std::is_const_v<S>) &&
         (N < storage_size_v<S>) &&
         std::constructible_from<storage_alternative_t<N, S>, Args...>
constexpr auto* storage_emplace(S& storage, Args&&... args)
noexcept(std::is_nothrow_constructible_v<storage_alternative_t<N, S>, Args...>) {
    return std::construct_at(storage_emplace_impl<N>(storage), std::forward<Args>(args)...);
}

} // namespace detail

} // namespace varerr

#endif // VARERR_STORAGE_HPP
