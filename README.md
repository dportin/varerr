# VarErr (Variadic Errors)

Experimental error-handling library.

## Dependencies

- The headers require a compiler with C++23 language support.
- The `-dev` presets require a generator that can emit `compile_commands.json`.
- The `-dev-tools` presets require Clang-Tidy and IWYU (see below for platform requirements).

## Quick Start

Build and run the tests using a preset:

    $ cmake --list-presets
    $ cmake -S . --preset $PRESET
    $ cmake --build --preset $PRESET
    $ ctest --preset $PRESET

The `-dev` presets select the `Debug` build type and enable the following options:

    varerr_TESTING         # Generate test targets
    varerr_INSTALL         # Generate install targets
    varerr_DEVELOPMENT     # Use strict compile options.
    varerr_VENDORED_CATCH2 # Download and build Catch2

The `-dev-tools` presets extend the corresponding `-dev` presets and generate test targets for the following static analysis tools:

    varerr_TESTING_CLANG_TIDY # Generate Clang-Tidy test targets
    varerr_TESTING_IWYU       # Generate IWYU test targets

Clang-Tidy is enabled for all configurations. IWYU is enabled only for Clang configurations on Linux.

## TODO

Regarding `volatile` qualifiers:

- `BasicStatus<M, volatile E, Es...>` should be supported.
- `BasicResult<M, volatile T, Es...>` should be supported.
- In general the rationale for prohibiting `volatile` on `BasicStatus` and `BasicResult` is that volatile wrapper objects are not supported by the STL and volatile objects cannot use their implicit copy and move constructors. But a volatile *value* or *error* need have no such restriction. The error case is ruled out by the plain type requirement. The value case is undecided.

Miscellaneous:

- Replace `IsRankedPack && IsTriviallyStorable` with `IsErrorRow` throughout.
- Define `IsUniverse` concept to validate existence of rank function.
- Revisit explicitness of `BasicResult` value constructors for ergonomics.
- Merge all four aliases of `visitor_argument_t` to `forwarded_argument_t`.
- Consider replacing `std::in_place_type_t` in `BasicResult` error constructor with custom `error_t`.
- Equality operator for `BasicStatus` and `BasicResult`.
- Move copy/move constructibility static assertions to class constraints for `BasicStatus` and `BasicResult`.
- Refactor `result.hpp` and implement tests.
- Move copy and move constructibility requirements into `BasicStatus` constraints.
- Remove the forwarding constructor from `BasicStatus`.
- Simplify compile-time iteration with `iterate_index_sequence` and friends.
- Simplify lifting operations with `pack_apply` and friends.
- Give traits in public interface more meaningful names (`IsTriviallyStorable` vs `IsStorageAlternative`).
- Provide a static factory method to in-place construct the error branch of `BasicResult`.
- Consider testing sparsely-ranked rows in `test_algebra.cpp`.
- Consider switching between jump table and binary dispatch in `BasicStatus` when number of alternatives is large.

## TODO (Cleanup)

- [P1286R2](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p1286r2.html) decouples the exception specification of an explicitly defaulted function from its triviality: `~E() noexcept(false) = default` is trivial but potentially throwing; `std::is_trivially_destructible_v<E>` no longer implies `std::is_nothrow_destructible_v<E>`. Since a trivial destructor has no body, the storage guarantees are unaffected and discarding an error cannot throw. However, any trait that tests nothrow destructibility as a proxy for those guarantees might disagree because it asks a narrower question. Revisit whether `IsTriviallyStorable` should additional require `std::is_nothrow_destructible_v`. This would restore the equivalence at the row boundary and reject such (degenerate) alternatives rather than admittin them and (potentially) failing elsewhere.

## TODO (Refactor)

- `BasicResult` should manage its own union and discriminator instead of delegating to `std::expected` (this would eliminate one redundant move without needing to pass the lambda in a suspension).

## License

This project is licensed under the [MIT](LICENSE) license.
