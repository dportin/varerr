#ifndef VARERR_TESTS_LIFETIME_HPP
#define VARERR_TESTS_LIFETIME_HPP

#include <cstddef>
#include <type_traits>
#include <utility>

namespace varerr::tests::lifetime {

struct Record {

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

    [[nodiscard]] friend constexpr bool operator==(const Record&, const Record&) noexcept = default;

};

template <typename T>
struct Tracked {

    Tracked() noexcept(std::is_nothrow_default_constructible_v<T>) :
        local_ {}, global_ {}, value_ {} {
        this->record(&Record::default_constructed);
    }

    explicit Tracked(Record* global) noexcept(std::is_nothrow_default_constructible_v<T>) :
        local_ {}, global_ { global }, value_ {} {
        this->record(&Record::default_constructed);
    }

    explicit Tracked(T value) noexcept(std::is_nothrow_move_constructible_v<T>) :
        local_ {}, global_ {}, value_ { std::move(value) } {
        this->record(&Record::value_constructed);
    }

    explicit Tracked(Record* global, T value) noexcept(std::is_nothrow_move_constructible_v<T>) :
        local_ {}, global_ { global }, value_ { std::move(value) } {
        this->record(&Record::value_constructed);
    }

    Tracked(const Tracked& other) noexcept(std::is_nothrow_copy_constructible_v<T>) :
        local_ { other.local_ }, global_ { other.global_ }, value_ { other.value_ } {
        this->record(&Record::copy_constructed);
    }

    Tracked(Tracked&& other) noexcept(std::is_nothrow_move_constructible_v<T>) :
        local_ { other.local_ }, global_ { other.global_ }, value_ { std::move(other.value_) } {
        this->record(&Record::move_constructed);
    }

    Tracked& operator=(const Tracked& other) noexcept(std::is_nothrow_copy_assignable_v<T>) {
        if (this != &other) {
            this->local_ = other.local_;
            this->value_ = other.value_;
        }
        this->record(&Record::copy_assigned);
        return *this;
    }

    Tracked& operator=(Tracked&& other) noexcept(std::is_nothrow_move_assignable_v<T>) {
        if (this != &other) {
            this->local_ = other.local_;
            this->value_ = std::move(other.value_);
        }
        this->record(&Record::move_assigned);
        return *this;
    }

    ~Tracked() noexcept {
        this->record(&Record::destructed);
    }

    [[nodiscard]] Record& local() noexcept {
        return this->local_;
    }

    [[nodiscard]] const Record& local() const noexcept {
        return this->local_;
    }

    [[nodiscard]] Record* global() noexcept {
        return this->global_;
    }

    [[nodiscard]] const Record* global() const noexcept {
        return this->global_;
    }

    [[nodiscard]] T& value() noexcept {
        return this->value_;
    }

    [[nodiscard]] const T& value() const noexcept {
        return this->value_;
    }

    private:

    void record(std::size_t Record::* field) noexcept {
        ++(this->local_.*field);
        if (this->global_) {
            ++(this->global_->*field);
        }
    }

    // The local record tracks the lifetime of the current object; the global
    // record persists across object lifetimes and can track destructor calls.

    Record local_;
    Record* global_;
    T value_;

};

} // namespace varerr::tests::lifetime

#endif // VARERR_TESTS_LIFETIME_HPP
