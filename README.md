# 🏎️ Apex Overdrive (v3.0) — Next-Gen Automotive Gaming Engine & Cluster

<p align="center">
  <img src="assets/logo.png" alt="Apex Overdrive Logo" width="180">
  <br>
  <b>High-Octane Desktop Game Optimizer with Automotive Gauge Cluster & PC Hardware Scanner</b>
  <br>
  <i>Cross-Platform (Windows 10/11 + Linux) • C++ Native Core • Zero Frame-Drops</i>
</p>

---

## 🌟 What is Apex Overdrive?

**Apex Overdrive** is a next-generation desktop gaming optimization suite and live telemetry cockpit. It combines low-level system engineering with an immersive **cyberpunk automotive racing cluster**, dynamic **sequential shift-light LEDs**, an automated **PC Hardware Capability Benchmark Scanner**, and the **Warp Drive Turbo Boost Engine**.

When you launch any game, Apex Overdrive automatically elevates process scheduling, trims background resource hogs, unparks CPU cores, lowers system timer resolution to 0.5ms, and tunes your GPU for maximum frame rates. When your game session concludes, it cleanly rolls back every setting to factory defaults.

---

## ⚡ Core Features

### 🏎️ Automotive Car Cluster Cockpit
- **Tachometer & Speedometer Dials**: Precision canvas-rendered dials for **CPU Load**, **GPU Engine**, and **RAM Boost Utilization** with realistic needle physics and redline warnings.
- **Sequential Shift-Light LED Bar**: 10-stage sequential racing shift lights (Green → Amber → Redline) reacting in real time to system workload.
- **Startup Ignition Gauge Sweep**: Authentic sports car gauge sweep sequence (0 → 100 → 0) on application startup.
- **Center HUD Telemetry**: Live readouts for estimated FPS boost, system multimedia timer resolution (0.5ms), power schemes, and GPU clock states.

### 💻 Automatic PC Hardware Benchmark Scanner
- **Deep Architecture Inspection**: Automatically inspects processor cores/threads/clocks, graphics card model/VRAM, memory capacity/speed/dual-channel mode, and NVMe SSD DirectStorage support.
- **Gaming Capability Benchmark**: Calculates your system's gaming benchmark score (0–100) and awards an official rank:
  - 🏆 **Tier S+ (Apex Titan)**
  - 🥇 **Tier S (Ultra High-End)**
  - 🥈 **Tier A (High Performance)**
  - 🥉 **Tier B (Esports Ready)**

### 🚀 Warp Drive Turbo Boost Engine
- **Aggressive CPU Turbo Scaling**: Locks processor performance boost mode to aggressive frequency scaling curves.
- **CPU Core Unparking (100% Cores Active)**: Bypasses Windows core parking, eliminating frame stutters during intensive gaming scenes.
- **0.5ms High-Precision Multimedia Timer**: Lowers OS scheduler quantum from standard 15.6ms to 0.5ms, eliminating micro-stutters and input lag.
- **Network Gaming Latency Optimizer**: Eliminates Nagle's algorithm delay (`TcpAckFrequency` / `TCPNoDelay`) for lowest online ping and jitter.
- **DirectX Discrete GPU Routing**: Forces graphics workloads strictly onto high-performance discrete GPUs.

### 🧹 Instant Standby RAM Purge
- **Pneumatic Blow-Off Valve**: Instant flush of standby memory cache and working set trimming with tactile audio feedback.
- **Safe Rollback**: Revert to Balanced/Factory defaults at any time with a single click.

### 1. Game Detection
- **Windows**: Fast process polling using `CreateToolhelp32Snapshot` / `Process32FirstW` / `Process32NextW` with full executable image path resolution via `QueryFullProcessImageNameA`.
- **Linux**: Efficient `/proc` scanning using `/proc/[pid]/comm` and `/proc/[pid]/exe` symlink dereferencing.
- **Dynamic Registration**: Support for adding any unlisted game by executable path via CLI (`--add-game`), system tray menu, or dashboard UI. Pre-seeded with profiles for major esports and AAA titles.

### 2. Process Management & Background Throttling
- Identifies resource-heavy non-essential background processes (Discord, Slack, Spotify, Chrome, Edge, Firefox, Epic Games Launcher, Steam Web Helper, OneDrive, torrent clients, etc.).
- **Windows**: Dynamically lowers background process priorities to `IDLE_PRIORITY_CLASS` during gameplay.
- **Linux**: Renices background processes to lower priority (`nice +15` to `+19`).
- **Safety Whitelist**: Strict immunity for critical operating system processes (Windows kernel, `explorer.exe`, `dwm.exe`, `csrss.exe`, `services.exe`, audio drivers; Linux `systemd`, `dbus`, display servers, and audio daemons).
- Stores original priorities and automatically restores them upon game exit.

### 3. CPU & GPU Tuning
- **Windows**:
  - Dynamically elevates the game process priority class to `HIGH_PRIORITY_CLASS`.
  - Sets CPU core affinity masks (`SetProcessAffinityMask`) to isolate games on performance cores or eliminate core hopping.
  - DirectX GPU Routing: Configures `HKCU\Software\Microsoft\DirectX\UserGpuPreferences` to ensure discrete high-performance GPU utilization.
  - Dynamically interfaces with NVIDIA NVAPI (`nvapi64.dll`/`nvapi.dll`) and AMD ADL (`atiadlxx.dll`) for power/clock states when drivers are installed.
- **Linux**:
  - Adjusts game process niceness (`nice -10`) or real-time scheduling where permissions allow.
  - Core pinning via `sched_setaffinity`.
  - CPU scaling governor management: Switches `/sys/devices/system/cpu/cpu*/cpufreq/scaling_governor` to `performance` (or via `cpupower` / `powerprofilesctl`).
  - Seamlessly integrates with Feral Interactive's **GameMode** (`libgamemode.so.0` / D-Bus) when present.

### 4. RAM Optimization
- **Windows**:
  - Trims working sets of background applications using `EmptyWorkingSet` and `SetProcessWorkingSetSize`.
  - Flushes the Windows NT standby memory list via `NtSetSystemInformation` (`MemoryPurgeStandbyList`).
- **Linux**:
  - Memory compaction via `/proc/sys/vm/compact_memory` to eliminate allocation fragmentation.
  - Destructive pagecache drop (`/proc/sys/vm/drop_caches`) is strictly **opt-in** per profile, executing `sync()` beforehand.

### 5. Power Plan Switching
- **Windows**: Queries the currently active power scheme (`PowerGetActiveScheme`), switches to High Performance (`GUID_MIN_POWER_SAVINGS`) or Ultimate Performance, and reverts to the exact original scheme on exit.
- **Linux**: Integrates with `powerprofilesctl set performance` or CPU governor scaling, restoring the previous profile on exit.

### 6. Crash Resilience & Safe Rollback
- **RAII Guards**: `OptimizationGuard` ensures state reversion during stack unwinding or abnormal scope exits.
- **Signal & Exception Handlers**: Traps `SIGINT`, `SIGTERM`, `SIGABRT`, `SIGSEGV` and Windows `SetConsoleCtrlHandler` / `SetUnhandledExceptionFilter`.
- **Persistent Rollback Journal**: Saves active modifications to disk (`active_session.json`). If the system loses power or terminates abnormally, the next startup detects the journal and automatically rolls back orphaned tweaks.
- **Restore Defaults**: Manual rollback command available at any time via `--restore` or system tray.

### 7. System Tray & Background Operation
- **Windows**: Lightweight native Win32 notification icon (`Shell_NotifyIconW`) with context menu, toast notifications, and dark-themed dashboard window. Zero heavy GUI framework dependencies.
- **Linux**: Background daemon mode (`--daemon`) with FreeDesktop notifications (`notify-send` / D-Bus).

---

## Project Structure

```
Game-Optimizer/
├── CMakeLists.txt                # Unified cross-platform CMake build configuration
├── CPackConfig.cmake             # CPack packaging for RPM, DEB, TGZ, NSIS, and ZIP
├── mingw64-toolchain.cmake       # MinGW cross-compilation toolchain file
├── build.bat                     # Windows automated build script
├── LICENSE                       # MIT License
├── README.md                     # Documentation and guides
├── include/
│   └── optimizer/
│       ├── Types.hpp             # Data models & zero-dependency JSON parser/serializer
│       ├── Logger.hpp            # Thread-safe colorized console & file logger
│       ├── IProcessManager.hpp   # Process priority & affinity interface
│       ├── IPowerManager.hpp     # Power scheme management interface
│       ├── IGpuController.hpp    # GPU clock & power state interface
│       ├── IRamOptimizer.hpp     # Working set & cache trimming interface
│       ├── IGameDetector.hpp     # Process detection interface
│       ├── ProfileManager.hpp    # Per-game profile manager & JSON persistence
│       ├── RollbackJournal.hpp   # Crash-safe rollback journal
│       ├── OptimizationSession.hpp# Transactional optimization & RAII guards
│       ├── SystemMonitor.hpp     # Live CPU & RAM metrics sampler
│       ├── OptimizerEngine.hpp   # Main orchestration engine
│       └── SystemTray.hpp        # Desktop tray abstraction
├── src/
│   ├── core/                     # Platform-independent implementations
│   │   ├── Logger.cpp
│   │   ├── ProfileManager.cpp
│   │   ├── RollbackJournal.cpp
│   │   ├── OptimizationSession.cpp
│   │   ├── SystemMonitor.cpp
│   │   └── OptimizerEngine.cpp
│   ├── platform/
│   │   ├── windows/              # Windows backend (Win32, NTAPI, PowerCfg, Tray)
│   │   │   ├── WinProcessManager.cpp
│   │   │   ├── WinPowerManager.cpp
│   │   │   ├── WinGpuController.cpp
│   │   │   ├── WinRamOptimizer.cpp
│   │   │   ├── WinGameDetector.cpp
│   │   │   └── WinSystemTray.cpp
│   │   └── linux/                # Linux backend (/proc, sched, sysfs, GameMode)
│   │       ├── LinuxProcessManager.cpp
│   │       ├── LinuxPowerManager.cpp
│   │       ├── LinuxGpuController.cpp
│   │       ├── LinuxRamOptimizer.cpp
│   │       ├── LinuxGameDetector.cpp
│   │       └── LinuxSystemTray.cpp
│   └── main.cpp                  # Application entry point & CLI handler
├── packaging/
│   ├── windows/
│   │   ├── app.manifest          # Windows UAC & compatibility manifest
│   │   └── installer.nsi.in      # NSIS Modern UI installer script template
│   ├── linux/
│   │   ├── gameoptimizer.desktop # FreeDesktop application launcher
│   │   ├── gameoptimizer.service # Systemd user unit file
│   │   └── org.gameoptimizer.GameOptimizer.yml # Flatpak manifest
│   └── profiles/
│       └── default_profiles.json # Default pre-seeded game database
└── tests/
    └── test_optimizer.cpp        # Automated unit and integration test suite
```

---

## Build Instructions

### Prerequisites
- **CMake**: version 3.20 or newer
- **C++ Compiler**: GCC 10+, Clang 11+, or MSVC 2019+ (C++17 support required)

### Building on Linux
```bash
# Configure with Ninja (or Unix Makefiles)
cmake -B build -S . -GNinja -DCMAKE_BUILD_TYPE=Release

# Build executable and test suite
cmake --build build

# Run unit tests
./build/test_optimizer
```

### Building on Windows (Native)
Run the included build script:
```cmd
build.bat
```
Or manually via CMake with MSVC or MinGW:
```cmd
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

### Cross-Compiling for Windows from Linux / WSL (MinGW)
```bash
cmake -B build-win -S . -GNinja -DCMAKE_TOOLCHAIN_FILE=mingw64-toolchain.cmake
cmake --build build-win
```

---

## Native Installers & Packaging

Native packages for both operating systems are generated using **CPack**:

### Linux Installers (.rpm, .deb, .tar.gz)
```bash
cd build
cpack -G "RPM;DEB;TGZ"
```
Generated artifacts:
- `game-optimizer-2.0.0-Linux.rpm` (Fedora, RHEL, openSUSE)
- `game-optimizer-2.0.0-Linux.deb` (Ubuntu, Debian, Linux Mint)
- `game-optimizer-2.0.0-Linux.tar.gz` (Portable binary archive)

### Windows Installers (.exe, .zip)
```bash
cd build-win
cpack -G "NSIS;ZIP"
```
Generated artifacts:
- `game-optimizer-2.0.0-win64.zip` (Portable standalone package)
- `game-optimizer-2.0.0-setup.exe` (NSIS native setup wizard with Start Menu shortcuts and uninstaller)

---

## Required Permissions & Graceful Degradation

Game Optimizer is designed around **graceful degradation**; missing privileges or unsupported hardware will log a warning and skip that individual tweak rather than crashing:

| Feature | Windows Privilege | Linux Privilege | Fallback Behavior |
| :--- | :--- | :--- | :--- |
| **Game Priority Elevation** | Standard user | Standard user (for +nice) / `CAP_SYS_NICE` (for -nice) | Runs at normal priority if elevation fails |
| **CPU Core Affinity** | Standard user | Standard user | Uses OS scheduler default |
| **Power Plan Switching** | Standard user | `powerprofilesctl` / `polkit` / `sudo` | Leaves power profile unchanged |
| **RAM Working Set Trim** | Standard user | Standard user | Skips if process inaccessible |
| **Standby Memory Flush** | Administrator (`SeProfileSingleProcessPrivilege`) | N/A | Skips flush with informative warning |
| **Linux Cache Drop** | N/A | `root` / `sudo` | Requires opt-in; skips if unprivileged |
| **GPU Clock Tuning** | Standard user | Driver privileges (`nvidia-smi` / `sysfs`) | Falls back to Windows DirectX GPU preference / GameMode |

---

## How to Add a New Game Profile

### 1. Via Command Line
To register an unlisted game, pass the full executable path:
```bash
# Windows
game-optimizer.exe --add-game "D:\SteamLibrary\steamapps\common\BlackMythWukong\b1.exe" "Black Myth: Wukong"

# Linux
game-optimizer --add-game "/home/user/.steam/steam/steamapps/common/Deadlock/game/bin/linuxsteamrt64/deadlock" "Deadlock"
```

### 2. Via Desktop System Tray / Dashboard
1. Right-click the **Game Optimizer** icon in the Windows notification area.
2. Select **Add Game Executable...**.
3. Browse and select the `.exe` file. The optimizer will automatically configure recommended profile settings and save them.

### 3. Via `profiles.json`
Profiles are stored in JSON format at:
- **Windows**: `%APPDATA%\GameOptimizer\profiles.json`
- **Linux**: `~/.config/gameoptimizer/profiles.json`

Example profile entry:
```json
{
  "name": "Custom Game",
  "exeName": "mygame.exe",
  "fullPath": "C:\\Games\\MyGame\\mygame.exe",
  "tuneCpuPriority": true,
  "targetPriority": "High",
  "tuneAffinity": false,
  "affinityMask": 0,
  "tuneGpu": true,
  "switchPowerPlan": true,
  "lowerBackgroundProcesses": true,
  "trimRamOnLaunch": true,
  "dropCachesOnLaunch": false
}
```

---

## Command Line Interface (CLI) Reference

| Option | Description |
| :--- | :--- |
| `--monitor` | Runs the active game detection and optimization loop in the foreground console |
| `--tray`, `--gui` | Launches the background system tray app with notification toasts and dashboard |
| `--daemon` | Runs as a background service/daemon |
| `--status` | Displays real-time CPU, RAM, GPU, active power plan, and current game status |
| `--list-games` | Lists all registered games and their configured optimizations |
| `--add-game <path> [name]` | Adds a new game by executable path |
| `--remove-game <name>` | Removes a registered game profile |
| `--optimize-now <pid>` | Manually applies optimizations to an already running process ID |
| `--restore` | Immediately reverts all active tweaks and restores system defaults |
| `--version`, `-v` | Prints application version information |
| `--help`, `-h` | Displays usage instructions and examples |

---

## License
Distributed under the MIT License. See [LICENSE](file:///c:/Users/jacky/Documents/GitHub/Game-Optimizer/LICENSE) for more information.
