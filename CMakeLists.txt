cmake_minimum_required(VERSION 3.24)

option(CMAKE_COMPILE_WARNING_AS_ERROR "Enable warnings-as-error" On)
option(FIN_BS_BUILD_TESTS "Enable build of the tests" Off)

if (FIN_BS_BUILD_TESTS)
    set(BUILD_TESTING On)
endif()

# numerical-toolbox only fetches embedded-infra-lib when it is the top-level project,
# so as its parent we have to provide infra.util and the emil_* helpers ourselves.
# Keep this pin in sync with the FetchContent_Declare in numerical-toolbox/CMakeLists.txt.
include(FetchContent)

FetchContent_Declare(
    emil
    GIT_REPOSITORY https://github.com/embedded-pro/embedded-infra-lib.git
    GIT_TAG        2848dbf163a6ebf159e29f7f4f73b96f7773d20d # Unreleased
)

set(EMIL_INCLUDE_MBEDTLS Off CACHE BOOL "" FORCE)
set(EMIL_INCLUDE_ECHO Off CACHE BOOL "" FORCE)
set(EMIL_FETCH_ECHO_COMPILERS Off CACHE BOOL "" FORCE)
set(EMIL_BUILD_ECHO_COMPILERS Off CACHE BOOL "" FORCE)
set(EMIL_ENABLE_DOCKER_TOOLS Off CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(emil)

project(fin_bs LANGUAGES C CXX VERSION 0.0.1) # x-release-please-version

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED On)

set_directory_properties(PROPERTY USE_FOLDERS ON)

if (FIN_BS_BUILD_TESTS)
    include(CTest)
    emil_enable_testing()
endif()

add_subdirectory(numerical-toolbox)
add_subdirectory(source)

emil_clangformat_directories(fin_bs DIRECTORIES source)
emil_folderize_all_targets()
emil_exclude_directory_from_coverage(numerical-toolbox)
emil_coverage_targets(DIRECTORIES source)
