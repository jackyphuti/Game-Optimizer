cmake_minimum_required(VERSION 3.20)
project(GameOptimizer VERSION 2.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings and options
if(MSVC)
    add_compile_options(/W4 /permissive- /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -O2)
endif()

if(MINGW)
    add_link_options(-static -static-libgcc -static-libstdc++)
endif()

# Core library sources
set(CORE_SOURCES
    src/core/Logger.cpp
    src/core/ProfileManager.cpp
    src/core/RollbackJournal.cpp
    src/core/OptimizationSession.cpp
    src/core/SystemMonitor.cpp
    src/core/OptimizerEngine.cpp
)

# Platform-specific backend sources
if(WIN32)
    set(PLATFORM_SOURCES
        src/platform/windows/WinProcessManager.cpp
        src/platform/windows/WinPowerManager.cpp
        src/platform/windows/WinGpuController.cpp
        src/platform/windows/WinRamOptimizer.cpp
        src/platform/windows/WinGameDetector.cpp
        src/platform/windows/WinSystemTray.cpp
    )
    set(PLATFORM_LIBS
        advapi32
        powrprof
        ole32
        comdlg32
        user32
        shell32
        gdi32
        psapi
    )
else()
    set(PLATFORM_SOURCES
        src/platform/linux/LinuxProcessManager.cpp
        src/platform/linux/LinuxPowerManager.cpp
        src/platform/linux/LinuxGpuController.cpp
        src/platform/linux/LinuxRamOptimizer.cpp
        src/platform/linux/LinuxGameDetector.cpp
        src/platform/linux/LinuxSystemTray.cpp
    )
    set(PLATFORM_LIBS
        pthread
        dl
    )
endif()

# Static library containing platform logic and core engine
add_library(game_optimizer_core STATIC
    ${CORE_SOURCES}
    ${PLATFORM_SOURCES}
)

target_include_directories(game_optimizer_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(game_optimizer_core PUBLIC
    ${PLATFORM_LIBS}
)

# Main desktop executable
add_executable(game-optimizer
    src/main.cpp
)

target_link_libraries(game-optimizer PRIVATE
    game_optimizer_core
)

# Apply Windows application manifest if compiling for Win32
if(WIN32 AND MSVC)
    set_target_properties(game-optimizer PROPERTIES
        LINK_FLAGS "/MANIFEST:EMBED /MANIFESTINPUT:\"${CMAKE_CURRENT_SOURCE_DIR}/packaging/windows/app.manifest\""
    )
endif()

# Automated Test Suite
option(BUILD_TESTS "Build automated unit and integration tests" ON)
if(BUILD_TESTS)
    enable_testing()
    add_executable(test_optimizer
        tests/test_optimizer.cpp
    )
    target_link_libraries(test_optimizer PRIVATE
        game_optimizer_core
    )
    add_test(NAME OptimizerTests COMMAND test_optimizer)
endif()

# Installation paths
include(GNUInstallDirs)

install(TARGETS game-optimizer
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(NOT WIN32)
    # Desktop integration on Linux
    install(FILES packaging/linux/gameoptimizer.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
    )
    install(FILES packaging/linux/gameoptimizer.service
        DESTINATION lib/systemd/user
    )
    install(FILES packaging/profiles/default_profiles.json
        DESTINATION ${CMAKE_INSTALL_DATADIR}/gameoptimizer
    )
else()
    install(FILES packaging/profiles/default_profiles.json
        DESTINATION bin
    )
endif()

# Include CPack packaging configuration
include(${CMAKE_CURRENT_SOURCE_DIR}/CPackConfig.cmake)
