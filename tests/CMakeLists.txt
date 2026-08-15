include(Catch)

# Generate Catch2 targets.

add_executable(${PROJECT_NAME}_tests_main
    "tests.cpp"
)

target_link_libraries(${PROJECT_NAME}_tests_main PRIVATE
    ${PROJECT_NAME}::header_only
    ${PROJECT_NAME}::development
    Catch2::Catch2WithMain
)

catch_discover_tests(${PROJECT_NAME}_tests_main)

# The headers must be instantiated in some object file to generate a compilation
# database for Clang-Tidy to consume.

add_library(${PROJECT_NAME}_tests_instantiate_build OBJECT
    "instantiate/instantiate.cpp"
)

target_link_libraries(${PROJECT_NAME}_tests_instantiate_build PRIVATE
    ${PROJECT_NAME}::header_only
    ${PROJECT_NAME}::development
)

# Suppress Clang-Tidy warnings about GCC's C++20 module-scanning flags.

set_target_properties(${PROJECT_NAME}_tests_instantiate_build PROPERTIES
    CXX_SCAN_FOR_MODULES OFF
)

add_test(NAME ${PROJECT_NAME}_tests_instantiate
    COMMAND "${CMAKE_COMMAND}"
    --build "${CMAKE_BINARY_DIR}"
    --target ${PROJECT_NAME}_tests_instantiate_build
)

set_tests_properties(${PROJECT_NAME}_tests_instantiate PROPERTIES
    FIXTURES_SETUP ${PROJECT_NAME}_tests_fixture_instantiate
)

# Generate Clang-Tidy test targets and integrate Clang-Tidy with CTest.

if(${PROJECT_NAME}_DEVELOPMENT)

    find_program(${PROJECT_NAME}_CLANG_TIDY_BINARY
        NAMES "clang-tidy" REQUIRED
    )

    # Suppress unknown warning and argument errors to prevent global warnings-
    # as-errors policy from rejecting GCC warning and MSVC compiler flags.

    set(${PROJECT_NAME}_CLANG_TIDY_COMMAND
        "${${PROJECT_NAME}_CLANG_TIDY_BINARY}"
        --quiet
        --warnings-as-errors=*
        --extra-arg-before=-Wno-unknown-warning-option
        --extra-arg-before=-Wno-unused-command-line-argument
        "--header-filter=${PROJECT_NAME}/"
        --config-file "${${PROJECT_NAME}_SOURCE_DIR}/.clang-tidy"
        -p "${CMAKE_BINARY_DIR}"
        "${CMAKE_CURRENT_LIST_DIR}/instantiate/instantiate.cpp"
    )

    add_custom_target(${PROJECT_NAME}_tests_clang_tidy
        COMMAND ${${PROJECT_NAME}_CLANG_TIDY_COMMAND}
        VERBATIM
    )

    add_test(NAME ${PROJECT_NAME}_tests_clang_tidy
        COMMAND ${${PROJECT_NAME}_CLANG_TIDY_COMMAND}
    )

    add_dependencies(${PROJECT_NAME}_tests_clang_tidy
        ${PROJECT_NAME}_tests_instantiate_build
    )

    set_tests_properties(${PROJECT_NAME}_tests_clang_tidy PROPERTIES
        FIXTURES_REQUIRED ${PROJECT_NAME}_tests_fixture_instantiate
    )

endif()
