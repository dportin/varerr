cmake_minimum_required(VERSION 4.3 FATAL_ERROR)

include(CMakeDependentOption)

project(varerr
    VERSION 0.0.1.0
    DESCRIPTION "Experimental error-handling library"
    HOMEPAGE_URL "https://github.com/dportin/varerr"
    SPDX_LICENSE "MIT"
    LANGUAGES CXX
)

# Reject in-source builds.

if ("${PROJECT_BINARY_DIR}" STREQUAL "${PROJECT_SOURCE_DIR}")
    message(FATAL_ERROR "${PROJECT_NAME}: in-source builds not permitted")
endif()

# Only generate install targets if project is top level.

option(${PROJECT_NAME}_INSTALL "Generate install targets" "${PROJECT_IS_TOP_LEVEL}")

# Only generate test targets if project is top level.

option(${PROJECT_NAME}_TESTING "Generate test targets" "${PROJECT_IS_TOP_LEVEL}")

# Enable strict build policy in development mode.

option(${PROJECT_NAME}_DEVELOPMENT "Enable strict build policies" "${PROJECT_IS_TOP_LEVEL}")

# Enable Clang-Tidy test targets.

cmake_dependent_option(
    ${PROJECT_NAME}_TESTING_CLANG_TIDY "Generate Clang-Tidy test targets" OFF
    "${PROJECT_NAME}_TESTING" OFF
)

# Enable IWYU test targets.

cmake_dependent_option(
    ${PROJECT_NAME}_TESTING_IWYU "Generate IWYU test targets" OFF
    "${PROJECT_NAME}_TESTING" OFF
)

# Generate compilation database.

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Cache FetchContent dependencies outside build directory.

if(PROJECT_IS_TOP_LEVEL)
    set(FETCHCONTENT_BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.cache/deps" CACHE PATH "FetchContent dependency cache")
endif()

# Disable module dependency scanning.

if(PROJECT_IS_TOP_LEVEL)
    set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
endif()

# Generate header isolation tests.

if(${PROJECT_NAME}_TESTING)
    set(CMAKE_VERIFY_INTERFACE_HEADER_SETS ON)
endif()

# Interface library for common development options.

add_library(${PROJECT_NAME}_development INTERFACE)
add_library(${PROJECT_NAME}::development ALIAS ${PROJECT_NAME}_development)

if (${PROJECT_NAME}_DEVELOPMENT)

    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")

        target_compile_options(${PROJECT_NAME}_development INTERFACE
            /W4
            /WX
            /external:anglebrackets
            /external:W0
            /external:templates-
            /permissive-
            /Zc:inline
            /Zc:preprocessor
            /Zc:u8EscapeEncoding
            /Zc:enumTypes
            /Zc:templateScope
            /Zc:__cplusplus
            /Zc:throwingNew
        )

    else()

        # Clang 18 disables -fdelayed-template-parsing when targeting MSVC at
        # C++20 or later: enabling /Zc:twoPhase here is inert and harmless.

        if (CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
            target_compile_options(${PROJECT_NAME}_development INTERFACE
                /W4
                /Zc:twoPhase
            )
        else()
            target_compile_options(${PROJECT_NAME}_development INTERFACE
                -Wall
                -Wextra
                -Werror
            )
        endif()

        target_compile_options(${PROJECT_NAME}_development INTERFACE
            -Wpedantic
            -Wshadow
            -Wconversion
            -Wsign-conversion
            -Wcast-qual
            -Wdouble-promotion
            -Wformat=2
            -Wundef
            -Wnull-dereference
            -Wimplicit-fallthrough
            -Wmisleading-indentation
            -Wold-style-cast
            -Wnon-virtual-dtor
            -Woverloaded-virtual
            -Wextra-semi
        )

        # Clang does not recognize these diagnostics.

        if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            target_compile_options(${PROJECT_NAME}_development INTERFACE
                -Wduplicated-cond
                -Wduplicated-branches
                -Wlogical-op
                -Wuseless-cast
            )
        endif()

        # GCC folds member, capture and parameter shadowing into -Wshadow while
        # Clang splits them across the -Wshadow-* option family.

        if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            target_compile_options(${PROJECT_NAME}_development INTERFACE
                -Wshadow-all
            )
        endif()

    endif()

endif()

# Expose headers as interface library.

add_library(${PROJECT_NAME}_header_only INTERFACE)
add_library(${PROJECT_NAME}::header_only ALIAS ${PROJECT_NAME}_header_only)

target_compile_features(${PROJECT_NAME}_header_only INTERFACE
    cxx_std_23
)

set_target_properties(${PROJECT_NAME}_header_only PROPERTIES
    EXPORT_NAME header_only
)

target_sources(${PROJECT_NAME}_header_only
    INTERFACE
        FILE_SET HEADERS
        BASE_DIRS
            "${CMAKE_CURRENT_SOURCE_DIR}/include"
        FILES
            "${CMAKE_CURRENT_SOURCE_DIR}/include/${PROJECT_NAME}/storage.hpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/include/${PROJECT_NAME}/algebra.hpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/include/${PROJECT_NAME}/status.hpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/include/${PROJECT_NAME}/result.hpp"
)

# Generate compiled tests.

if(${PROJECT_NAME}_TESTING)
    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/FetchCatch2.cmake")
    enable_testing()
    add_subdirectory(tests)
endif()

# Generate install targets.

if(${PROJECT_NAME}_INSTALL)

    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)

    install(TARGETS ${PROJECT_NAME}_header_only
        EXPORT ${PROJECT_NAME}-targets
        FILE_SET HEADERS
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    )

    install(EXPORT ${PROJECT_NAME}-targets
        FILE ${PROJECT_NAME}-targets.cmake
        NAMESPACE ${PROJECT_NAME}::
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
    )

    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/${PROJECT_NAME}-config.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake"
        INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
    )

    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake"
        VERSION "${PROJECT_VERSION}"
        COMPATIBILITY SameMinorVersion
        ARCH_INDEPENDENT
    )

    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
    )

endif()
