include(Catch)


# Generate Catch2 targets.

add_executable(${PROJECT_NAME}_tests_main
    "test_main.cpp"
    "test_storage.cpp"
    "test_algebra.cpp"
)

target_link_libraries(${PROJECT_NAME}_tests_main PRIVATE
    ${PROJECT_NAME}::header_only
    ${PROJECT_NAME}::development
    Catch2::Catch2WithMain
)

catch_discover_tests(${PROJECT_NAME}_tests_main)

# Instantiate headers in object file to generate compilation database.

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

if(${PROJECT_NAME}_TESTING AND ${PROJECT_NAME}_TESTING_CLANG_TIDY)

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

# Generate IWYU test targets and integrate IWYU with CTest.

if(${PROJECT_NAME}_TESTING AND ${PROJECT_NAME}_TESTING_IWYU)

    find_program(${PROJECT_NAME}_IWYU_BINARY
        NAMES "include-what-you-use" "iwyu" REQUIRED
    )

    get_target_property(${PROJECT_NAME}_IWYU_HEADERS
        ${PROJECT_NAME}_header_only HEADER_SET
    )

    get_target_property(${PROJECT_NAME}_IWYU_HEADER_DIRS
        ${PROJECT_NAME}_header_only HEADER_DIRS
    )

    set(${PROJECT_NAME}_IWYU_INCLUDE_DIRS "")
    foreach(_IWYU_HEADER_DIR IN LISTS ${PROJECT_NAME}_IWYU_HEADER_DIRS)
        list(APPEND ${PROJECT_NAME}_IWYU_INCLUDE_DIRS "-I${_IWYU_HEADER_DIR}")
    endforeach()

    add_custom_target(${PROJECT_NAME}_tests_iwyu)

    foreach(_IWYU_HEADER IN LISTS ${PROJECT_NAME}_IWYU_HEADERS)

        unset(_IWYU_HEADER_RELATIVE)
        foreach(_IWYU_HEADER_DIR IN LISTS ${PROJECT_NAME}_IWYU_HEADER_DIRS)
            cmake_path(IS_PREFIX _IWYU_HEADER_DIR "${_IWYU_HEADER}" NORMALIZE _IWYU_HEADER_IS_RELATIVE)
            if(_IWYU_HEADER_IS_RELATIVE)
                cmake_path(RELATIVE_PATH _IWYU_HEADER
                    BASE_DIRECTORY "${_IWYU_HEADER_DIR}"
                    OUTPUT_VARIABLE _IWYU_HEADER_RELATIVE
                )
                break()
            endif()
        endforeach()

        if(NOT DEFINED _IWYU_HEADER_RELATIVE)
            message(FATAL_ERROR "${PROJECT_NAME}: header outside header base directories: ${_IWYU_HEADER}")
        endif()

        # Generate one translation unit per header.

        cmake_path(REMOVE_EXTENSION _IWYU_HEADER_RELATIVE OUTPUT_VARIABLE _IWYU_HEADER_RELATIVE_STEM)
        set(_IWYU_HEADER_RELATIVE_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/iwyu/${_IWYU_HEADER_RELATIVE_STEM}.cpp")

        file(CONFIGURE
            OUTPUT "${_IWYU_HEADER_RELATIVE_SOURCE}"
            CONTENT "#include <@_IWYU_HEADER_RELATIVE@> // IWYU pragma: keep\n"
            @ONLY
        )

        # Add "-Xiwyu --verbose=7" to debug IWYU errors.

        set(_IWYU_HEADER_TEST_COMMAND
            "${${PROJECT_NAME}_IWYU_BINARY}"
            -std=c++23
            ${${PROJECT_NAME}_IWYU_INCLUDE_DIRS}
            -Xiwyu --error
            -Xiwyu "--check_also=${_IWYU_HEADER}"
            "${_IWYU_HEADER_RELATIVE_SOURCE}"
        )

        string(REPLACE "/" "_" _IWYU_HEADER_RELATIVE_SUFFIX "${_IWYU_HEADER_RELATIVE_STEM}")

        add_test(NAME "${PROJECT_NAME}_tests_iwyu_${_IWYU_HEADER_RELATIVE_SUFFIX}"
            COMMAND ${_IWYU_HEADER_TEST_COMMAND}
        )

        add_custom_target(${PROJECT_NAME}_tests_iwyu_${_IWYU_HEADER_RELATIVE_SUFFIX}
            COMMAND ${_IWYU_HEADER_TEST_COMMAND}
            VERBATIM
        )

        add_dependencies(${PROJECT_NAME}_tests_iwyu
            ${PROJECT_NAME}_tests_iwyu_${_IWYU_HEADER_RELATIVE_SUFFIX}
        )

    endforeach()

endif()
