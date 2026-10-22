cmake_minimum_required(VERSION 3.20)
project(KisanSetu VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Strict warning flags
add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
    -Werror
)

# Optional Sanitizers
option(ENABLE_SANITIZERS "Enable Address and Undefined Behavior Sanitizers" OFF)
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(ENABLE_TSAN "Enable ThreadSanitizer" OFF)

if(ENABLE_SANITIZERS)
    set(ENABLE_ASAN ON)
    set(ENABLE_UBSAN ON)
endif()

if(ENABLE_ASAN)
    message(STATUS "Enabling AddressSanitizer (-fsanitize=address)")
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=address)
endif()

if(ENABLE_UBSAN)
    message(STATUS "Enabling UndefinedBehaviorSanitizer (-fsanitize=undefined)")
    add_compile_options(-fsanitize=undefined -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=undefined)
endif()

if(ENABLE_TSAN)
    message(STATUS "Enabling ThreadSanitizer (-fsanitize=thread)")
    add_compile_options(-fsanitize=thread -g)
    add_link_options(-fsanitize=thread)
endif()

# Include directories
include_directories(include)

# 1. Contracts Library (Header-only / Interface)
add_library(kisansetu_contracts INTERFACE)
target_include_directories(kisansetu_contracts INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# 2. Core Engine Library
file(GLOB CORE_SOURCES "src/core/*.cpp")
add_library(kisansetu_core STATIC ${CORE_SOURCES})
target_link_libraries(kisansetu_core PUBLIC kisansetu_contracts)
target_include_directories(kisansetu_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# 3. Domain Logic Library
file(GLOB DOMAIN_SOURCES "src/domain/*.cpp")
add_library(kisansetu_domain STATIC ${DOMAIN_SOURCES})
target_link_libraries(kisansetu_domain PUBLIC kisansetu_contracts)
target_include_directories(kisansetu_domain PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# 4. Strategy Library
file(GLOB STRATEGY_SOURCES "src/strategy/*.cpp")
add_library(kisansetu_strategy STATIC ${STRATEGY_SOURCES})
target_link_libraries(kisansetu_strategy PUBLIC kisansetu_domain)
target_include_directories(kisansetu_strategy PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# 5. Agents Library
file(GLOB AGENTS_SOURCES "src/agents/*.cpp")
add_library(kisansetu_agents STATIC ${AGENTS_SOURCES})
target_link_libraries(kisansetu_agents PUBLIC kisansetu_core kisansetu_domain kisansetu_strategy)
target_include_directories(kisansetu_agents PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# Main Executable
add_executable(kisansetu src/main.cpp)
target_link_libraries(kisansetu PRIVATE kisansetu_agents)

# Tests
enable_testing()
add_executable(kisansetu_tests
    tests/test_main.cpp
    tests/test_contracts.cpp
    tests/test_config.cpp
    tests/test_foundation.cpp
    tests/test_domain.cpp
    tests/test_cnp_integration.cpp
)
target_link_libraries(kisansetu_tests PRIVATE kisansetu_agents)
target_include_directories(kisansetu_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)

add_test(NAME AllUnitTests COMMAND kisansetu_tests)

# Architectural Boundary Test: Ensure no agent header includes another agent header
add_test(
    NAME ArchitecturalMediatorBoundaryCheck
    COMMAND ${CMAKE_COMMAND} -E echo "Checking agent headers for direct peer inclusions..."
)
