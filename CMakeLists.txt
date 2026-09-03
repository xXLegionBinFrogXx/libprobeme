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

set(PME_LIB_TARGETS "")

# ---------------------------------------------------------------------------
# linux provider (requires Linux: procfs/sysfs/statvfs)
# ---------------------------------------------------------------------------
if(PME_IS_LINUX AND "linux" IN_LIST PME_PROVIDERS)
    add_library(probeme_linux SHARED
        src/linux/provider_linux.c
        src/linux/procfs.c
        src/linux/cpu.c
        src/linux/memory.c
        src/linux/loadavg.c
        src/linux/uptime.c
        src/linux/disk_io.c
        src/linux/netdev.c
        src/linux/filesystem.c
        src/linux/thermal.c
    )
    target_link_libraries(probeme_linux PRIVATE probeme_common)
    set_target_properties(probeme_linux PROPERTIES
        OUTPUT_NAME probeme_linux
        VERSION ${PROJECT_VERSION}
        SOVERSION 1
    )
    list(APPEND PME_LIB_TARGETS probeme_linux)

    add_executable(dump examples/dump.c)
    target_include_directories(dump PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_libraries(dump PRIVATE ${CMAKE_DL_LIBS})
endif()

# ---------------------------------------------------------------------------
# nvml provider (requires Linux; plain NVML usage. When libnvidia-ml is not
# present at build time the .so builds with undefined symbols and fails only
# at dlopen time on machines without the driver)
# ---------------------------------------------------------------------------
if(PME_IS_LINUX AND "nvml" IN_LIST PME_PROVIDERS)
    add_library(probeme_nvml SHARED
        src/nvml/provider_nvml.c
        src/nvml/gpu.c
    )
    target_include_directories(probeme_nvml PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/nvml
    )
    target_link_libraries(probeme_nvml PRIVATE probeme_common)
    find_library(PME_NVML_LIBRARY nvidia-ml)
    if(PME_NVML_LIBRARY)
        target_link_libraries(probeme_nvml PRIVATE ${PME_NVML_LIBRARY})
        message(STATUS "probeme: linking NVML: ${PME_NVML_LIBRARY}")
    else()
        message(STATUS "probeme: libnvidia-ml not found; probeme_nvml resolves symbols at load time")
    endif()
    set_target_properties(probeme_nvml PROPERTIES
        OUTPUT_NAME probeme_nvml
        VERSION ${PROJECT_VERSION}
        SOVERSION 1
    )
    list(APPEND PME_LIB_TARGETS probeme_nvml)

    add_executable(live_nvml tests/live/live_nvml.c)
    target_compile_definitions(live_nvml PRIVATE
        PME_BUILD_LIB_DIR="${CMAKE_CURRENT_BINARY_DIR}"
    )
    target_include_directories(live_nvml PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_libraries(live_nvml PRIVATE ${CMAKE_DL_LIBS})
    add_test(NAME live_nvml COMMAND live_nvml)
    set_tests_properties(live_nvml PROPERTIES SKIP_RETURN_CODE 77)
endif()

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
enable_testing()

add_executable(test_abi_layout tests/abi/layout.c)
target_include_directories(test_abi_layout PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
add_test(NAME abi_layout COMMAND test_abi_layout)

# ---------------------------------------------------------------------------
# Unit tests (pure /proc parsers; run on any host, no fixtures required --
# committed fixture dirs under tests/fixtures/linux are picked up if present)
# ---------------------------------------------------------------------------
if("linux" IN_LIST PME_PROVIDERS)
    set(PME_UNIT_TESTS
        test_procfs_stat
        test_procfs_meminfo
        test_procfs_loadavg
        test_procfs_uptime
        test_procfs_diskstats
        test_procfs_net_dev
        test_procfs_mounts
    )
    foreach(t ${PME_UNIT_TESTS})
        add_executable(${t} tests/unit/${t}.c src/linux/procfs.c)
        target_include_directories(${t} PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${CMAKE_CURRENT_SOURCE_DIR}/src/linux
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/unit
        )
        target_compile_definitions(${t} PRIVATE
            PME_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/linux"
        )
        add_test(NAME ${t} COMMAND ${t})
        set_tests_properties(${t} PROPERTIES SKIP_RETURN_CODE 77)
    endforeach()
endif()

# ---------------------------------------------------------------------------
# Live tests: dlopen the built provider(s); Linux only, skip (77) when the
# provider cannot be loaded (e.g. no GPU for live_nvml)
# ---------------------------------------------------------------------------
if(PME_IS_LINUX)
    add_executable(live_linux tests/live/live_linux.c)
    target_compile_definitions(live_linux PRIVATE
        PME_BUILD_LIB_DIR="${CMAKE_CURRENT_BINARY_DIR}"
    )
    target_include_directories(live_linux PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_libraries(live_linux PRIVATE ${CMAKE_DL_LIBS})
    add_test(NAME live_linux COMMAND live_linux)
    set_tests_properties(live_linux PROPERTIES SKIP_RETURN_CODE 77)
endif()

# ---------------------------------------------------------------------------
# Install
# ---------------------------------------------------------------------------
include(GNUInstallDirs)
install(TARGETS ${PME_LIB_TARGETS}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES include/probeme.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
configure_file(cmake/probeme.pc.in ${CMAKE_CURRENT_BINARY_DIR}/probeme.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/probeme.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
