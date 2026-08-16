# VarErr (Variadic Errors)

Experimental error-handling library.

## Dependencies

- Compiling the headers requires a compiler with C++23 language support.
- Running the tests requires a generator that can emit `compile_commands.json`.

## Quick Start

Build and run the tests using a preset:

    $ cmake --list-presets
    $ cmake -S . --preset $PRESET
    $ cmake --build --preset $PRESET
    $ ctest --preset $PRESET

The presets select the `Debug` build type and enable the following options:

    varerr_TESTING         # Generate test targets
    varerr_INSTALL         # Generate install targets
    varerr_DEVELOPMENT     # Use strict compile options
    varerr_VENDORED_CATCH2 # Download and build Catch2

## TODO

- Tests for `storage_get`, `storage_activate` and `storage_emplace` primitives.
