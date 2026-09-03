cmake_minimum_required(VERSION 3.20)

project(probeme
    VERSION 1.0.0
    DESCRIPTION "System information snapshot library for Linux"
    LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

add_compile_definitions(_GNU_SOURCE)

include(cmake/warnings.cmake)

# Providers to build: subset of "linux;nvml".
set(PME_PROVIDERS "linux;nvml" CACHE STRING "Provider backends to build (linux;nvml)")

set(PME_IS_LINUX OFF)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(PME_IS_LINUX ON)
endif()

# ---------------------------------------------------------------------------
# Common object library: exported ABI symbols + helpers shared by providers.
# ---------------------------------------------------------------------------
add_library(probeme_common OBJECT
    src/common/provider.c
    src/common/clock.c
)
target_include_directories(probeme_common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
enable_testing()

add_executable(test_abi_layout tests/abi/layout.c)
target_include_directories(test_abi_layout PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
add_test(NAME abi_layout COMMAND test_abi_layout)

# ---------------------------------------------------------------------------
# Install
# ---------------------------------------------------------------------------
include(GNUInstallDirs)
install(FILES include/probeme.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
configure_file(cmake/probeme.pc.in ${CMAKE_CURRENT_BINARY_DIR}/probeme.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/probeme.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
