#ifndef VARERR_TESTS_LIFETIME_HPP
#define VARERR_TESTS_LIFETIME_HPP

#include <cstddef>
#include <utility>
#include <type_traits>

namespace lifetime {

struct Counts {

    std::size_t default_constructed {};
    std::size_t value_constructed {};
    std::size_t copy_constructed {};
    std::size_t move_constructed {};
    std::size_t copy_assigned {};
    std::size_t move_assigned {};
    std::size_t destructed {};

    [[nodiscard]] constexpr std::size_t copied() const noexcept {
        return copy_constructed + copy_assigned;
    }

    [[nodiscard]] constexpr std::size_t moved() const noexcept {
        return move_constructed + move_assigned;
    } 

    [[nodiscard]] constexpr std::size_t constructed() const noexcept {
        return default_constructed + value_constructed + copy_constructed + move_constructed;
    }

    [[nodiscard]] friend constexpr bool operator==(const Counts&, const Counts&) noexcept = default;

};

template <typename T>
struct Tracked {

    Tracked() noexcept(std::is_nothrow_default_constructible_v<T>) :
        local_ {}, global_ {}, value_ {} {
        this->record(&Counts::default_constructed);
    }

    explicit Tracked(Counts* global) noexcept(std::is_nothrow_default_constructible_v<T>) :
        local_ {}, global_ { global }, value_ {} {
        this->record(&Counts::default_constructed);
    }

    explicit Tracked(T value) noexcept(std::is_nothrow_move_constructible_v<T>) :
        local_ {}, global_ {}, value_ { std::move(value) } {
        this->record(&Counts::value_constructed);
    }

    explicit Tracked(Counts* global, T value) noexcept(std::is_nothrow_move_constructible_v<T>) :
        local_ {}, global_ { global }, value_ { std::move(value) } {
        this->record(&Counts::value_constructed);
    }

    Tracked(const Tracked& other) noexcept(std::is_nothrow_copy_constructible_v<T>) :
        local_ { other.local_ }, global_ { other.global_ }, value_ { other.value_ } {
        this->record(&Counts::copy_constructed);
    }

    Tracked(Tracked&& other) noexcept(std::is_nothrow_move_constructible_v<T>) :
        local_ { other.local_ }, global_ { other.global_ }, value_ { std::move(other.value_) } {
        this->record(&Counts::move_constructed);
    }

    Tracked& operator=(const Tracked& other) noexcept(std::is_nothrow_copy_assignable_v<T>) {
        if (this != &other) {
            this->local_ = other.local_;
            this->value_ = other.value_;
        }
        this->record(&Counts::copy_assigned);
        return *this;
    }

    Tracked& operator=(Tracked&& other) noexcept(std::is_nothrow_move_assignable_v<T>) {
        if (this != &other) {
            this->local_ = other.local_;
            this->value_ = std::move(other.value_);
        }
        this->record(&Counts::move_assigned);
        return *this;
    }

    ~Tracked() {
        this->record(&Counts::destructed);
    }

    [[nodiscard]] Counts& local() noexcept {
        return this->local_;
    }

    [[nodiscard]] const Counts& local() const noexcept {
        return this->local_;
    }

    [[nodiscard]] Counts* global() noexcept {
        return this->global_;
    }

    [[nodiscard]] const Counts* global() const noexcept {
        return this->global_;
    } 

    [[nodiscard]] T& value() noexcept {
        return this->value_;
    }

    [[nodiscard]] const T& value() const noexcept {
        return this->value_;
    }

    private:

    void record(std::size_t Counts::* field) noexcept {
        ++(this->local_.*field);
        if (this->global_) {
            ++(this->global_->*field);
        }
    }

    Counts local_ {}; /* local ledger tracks value*/
    Counts* global_ {}; /* global ledger tracks destructor calls */
    T value_ {};

};

} // namespace lifetime

#endif // VARERR_TESTS_LIFETIME_HPP
