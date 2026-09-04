# VarErr (Variadic Errors)

Experimental error-handling library.

## Dependencies

- The headers require a compiler with C++23 language support.
- The `-dev` presets require a generator that can emit `compile_commands.json`.

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

- Simplify compile-time functions with `iterate_index_sequence` and friends.
- Simplify lifting operations with `pack_apply`.
- Consider throwing when the rank function is not injective.
- Give traits and concepts in public interface more meaningful names.
- Give a static factory method to in-place construct the error branch of `BasicResult`.
- The `BasicStatus` discriminant should depend on the row size (`std::uint8_t` for small rows).
- Move pack indexing selection in `algebra.hpp` to a utility header and permit selection of implementation with a preprocessor define.


## License

This project is licensed under the [MIT](LICENSE) license.
