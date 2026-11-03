cmake_minimum_required(VERSION 3.20)

# Automatic vcpkg toolchain file discovery if not explicitly specified
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    if(DEFINED ENV{VCPKG_ROOT} AND EXISTS "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
        set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "vcpkg toolchain file")
    elseif(EXISTS "C:/vcpkg/scripts/buildsystems/vcpkg.cmake")
        set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "vcpkg toolchain file")
    endif()
endif()

# Option to build unit and integration tests
option(BUILD_TESTING "Build the test suite" ON)

# Enable vcpkg test features when testing is enabled
if(BUILD_TESTING)
    list(APPEND VCPKG_MANIFEST_FEATURES "tests")
endif()

project(LibraryManagementSystem
    VERSION 1.0.0
    DESCRIPTION "Modern C++20 Library Management System"
    LANGUAGES CXX
)

# Enforce C++20 standard globally
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find SQLite3 dependency managed by vcpkg
find_package(unofficial-sqlite3 CONFIG QUIET)
if(TARGET unofficial::sqlite3::sqlite3)
    set(SQLITE3_TARGET unofficial::sqlite3::sqlite3)
elseif(TARGET unofficial-sqlite3::sqlite3)
    set(SQLITE3_TARGET unofficial-sqlite3::sqlite3)
else()
    find_package(SQLite3 REQUIRED)
    set(SQLITE3_TARGET SQLite3::SQLite3)
endif()

# -----------------------------------------------------------------------------
# Reusable Core Library Target
# -----------------------------------------------------------------------------
add_library(library_management_lib STATIC
    src/database/DatabaseConnection.cpp
    src/database/SchemaManager.cpp
    src/models/Book.cpp
    src/models/BookCopy.cpp
    src/models/BookCopyStatus.cpp
    src/models/Member.cpp
    src/models/MemberStatus.cpp
    src/repositories/BookRepository.cpp
    src/repositories/BookCopyRepository.cpp
    src/repositories/MemberRepository.cpp
    src/services/BookService.cpp
    src/services/BookCopyService.cpp
    src/services/MemberService.cpp
    src/utils/ValidationUtils.cpp
)

target_include_directories(library_management_lib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(library_management_lib PUBLIC cxx_std_20)

target_link_libraries(library_management_lib
    PUBLIC
        ${SQLITE3_TARGET}
)

# Compiler-specific warnings and flags (MSVC)
if(MSVC)
    target_compile_options(library_management_lib
        PRIVATE
            /W4
            /permissive-
            /EHsc
            /utf-8
    )
endif()

# -----------------------------------------------------------------------------
# CLI Application Executable Target
# -----------------------------------------------------------------------------
add_executable(library_management_app
    src/main.cpp
)

set_target_properties(library_management_app PROPERTIES
    OUTPUT_NAME "library_management_system"
)

target_link_libraries(library_management_app
    PRIVATE
        library_management_lib
)

target_compile_features(library_management_app PRIVATE cxx_std_20)

if(MSVC)
    target_compile_options(library_management_app
        PRIVATE
            /W4
            /permissive-
            /EHsc
            /utf-8
    )
endif()

# -----------------------------------------------------------------------------
# Testing Configuration
# -----------------------------------------------------------------------------
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()

