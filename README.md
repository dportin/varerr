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

## License

This project is licensed under the [MIT](LICENSE) license.
