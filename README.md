# VarErr (Variadic Errors)

Experimental error-handling library.

## Dependencies

- The headers require a compiler with C++23 language support.
- The `-dev` presets require a generator that can emit `compile_commands.json` .

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

- Tests for `storage_get`, `storage_activate` and `storage_emplace` primitives.
- Consider throwing when `rank_v<R, E> == rank_v<R, F>` but `!std::same_as<E, F>` when normalizing rows.
- Hide everything in `status.hpp` except `IsRankedPack` (`IsErrorRow`) and `IsTriviallyStorable`.
- Consider providing a static factory method to in-place construct the error branch of BasicResult.
- Missing uniqueness/de-duplication coverage for `row_normalize_t`.
- Simplify some compile-time functions with `constexpr_for` (`test_algebra.cpp`).

- Names in test cases (currently just prints tuple index).
