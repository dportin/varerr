include_guard(GLOBAL)

option(${PROJECT_NAME}_VENDORED_CATCH2 "Fetch Catch2 with FetchContent instead of find_package" ON)

if(${PROJECT_NAME}_VENDORED_CATCH2)

    include(FetchContent)

    set(${PROJECT_NAME}_VENDORED_CATCH2_ARCHIVE_URI "https://github.com/catchorg/Catch2/archive/refs/tags/v3.15.3.tar.gz" CACHE STRING "Catch2 archive URI")
    set(${PROJECT_NAME}_VENDORED_CATCH2_ARCHIVE_HASH "SHA256=b0299ae552918220a7a6e21e7de5b714777f4e8c883fb70c4bb23fe01df8c6e3" CACHE STRING "Catch2 archive hash")

    FetchContent_Declare(catch2
        URL "${${PROJECT_NAME}_VENDORED_CATCH2_ARCHIVE_URI}"
        URL_HASH "${${PROJECT_NAME}_VENDORED_CATCH2_ARCHIVE_HASH}"
        DOWNLOAD_NO_PROGRESS TRUE
        DOWNLOAD_EXTRACT_TIMESTAMP FALSE
        EXCLUDE_FROM_ALL
        SYSTEM
    )

    set(CATCH_INSTALL_DOCS OFF CACHE BOOL "" FORCE)
    set(CATCH_INSTALL_EXTRAS OFF CACHE BOOL "" FORCE)
    set(CATCH_DEVELOPMENT_BUILD OFF CACHE BOOL "" FORCE)
    set(CATCH_ENABLE_REPRODUCIBLE_BUILD ON CACHE BOOL "" FORCE)

    FetchContent_MakeAvailable(catch2)

    # Update module path with Catch2 extras subdirectory.
    # https://github.com/catchorg/Catch2/blob/devel/docs/cmake-integration.md#usage

    list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)

else()

    find_package(Catch2 3 REQUIRED)

endif()
