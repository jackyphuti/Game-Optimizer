# 🏎️ Apex Overdrive v3.0.1 — Next-Gen Gaming Engine & Cluster

Welcome to **Apex Overdrive v3.0.1**, the complete high-octane desktop gaming optimization cockpit.

---

## ⚡ What's New in v3.0.1 (Hotfix Update)
- **Turbo Boost Execution Fix**: Fixed Windows `spawn ENOENT` error when engaging Turbo Boost / Warp Drive by properly unpacking native binaries to `resources/native/` and `app.asar.unpacked/native/`.
- **Safe Fallback & Exception Handling**: Wrapped all native process calls with robust error guards and OS-level power/scheduler fallbacks, preventing unhandled JavaScript dialog popups.
- **Enhanced Power & Timer Resolution**: Improved 0.5ms multimedia scheduling quantum activation and CPU core unparking routines.

---

## 🌟 Core Features

### 🏁 Automotive Car Cluster Cockpit
- **Triple-Dial Precision Cluster**: Real-time tachometer & speedometer dials for **CPU Load**, **GPU Engine Core**, and **RAM Boost Utilization**.
- **Sequential Shift-Light LED Bar**: 10-stage sequential racing shift lights (Green → Amber → Redline) reacting dynamically to system load.
- **Ignition Gauge Sweep**: Authentic sports car startup sequence where needles sweep from 0 to redline and back upon launch.
- **Center HUD Telemetry**: Real-time readouts for estimated FPS boost, system multimedia timer resolution, power schemes, and GPU clock states.

### 💻 Automatic PC Hardware Benchmark Scanner
- **Full Architecture Inspection**: Analyzes CPU model, cores, boost clock; GPU model, VRAM bandwidth, and driver; RAM capacity, speed, and channel config; and NVMe SSD DirectStorage readiness.
- **Gaming Capability Benchmark**: Automatically calculates a PC Gaming Benchmark Score (0–100) and assigns an official tier:
  - 🏆 **Tier S+ (Apex Titan)**
  - 🥇 **Tier S (Ultra High-End)**
  - 🥈 **Tier A (High Performance)**
  - 🥉 **Tier B (Esports Ready)**

### 🚀 Warp Drive Turbo Boost Engine
- **Aggressive CPU Turbo Scaling**: Bypasses throttling and locks boost frequency curves during gaming sessions.
- **CPU Core Unparking**: Forces all logical cores active (100% capacity), eliminating frame drops and micro-stutters.
- **0.5ms High-Precision Multimedia Timer**: Lowers scheduler quantum from 15.6ms to 0.5ms for lowest input latency and smooth frame pacing.
- **Network Gaming Latency Optimizer**: Eliminates Nagle's algorithm delay (`TcpAckFrequency` & `TCPNoDelay`) to reduce ping and packet jitter.
- **GPU Overdrive Profile**: Forces discrete GPU routing and maximum performance clock profiles (P0 state).

### 🧹 Instant Standby RAM Purge
- **Pneumatic Blow-Off Valve**: Instant flush of standby memory cache and working set trimming without requiring a reboot.
- **One-Click Revert**: Complete RAII safety architecture to restore Windows / Linux defaults at any time.

### 🎮 Game Vault Library
- Auto-detects installed titles (CS2, Cyberpunk 2077, Valorant, Dota 2, Apex Legends, GTA V, Elden Ring, Warzone, etc.) with one-click boost on launch.

---

## 📦 Downloads & Installation

| Platform | Package | Description |
| :--- | :--- | :--- |
| **Windows 10/11** | `Apex Overdrive Setup 3.0.1.exe` | Standard Windows Installer with Desktop & Start Menu shortcuts |
| **Windows 10/11** | `Apex Overdrive 3.0.1.exe` | Portable Standalone Executable (No installation needed) |
| **Linux (Debian/Ubuntu)** | `apex-overdrive_3.0.1_amd64.deb` | Debian / Ubuntu / Mint package |
| **Linux (Fedora/RHEL)** | `apex-overdrive-3.0.1.x86_64.rpm` | RedHat / Fedora / openSUSE package |
| **Linux (Universal)** | `apex-overdrive-3.0.1.tar.gz` | Universal Linux portable binary archive |

---

## 🛠️ System Requirements
- **Windows**: Windows 10 or 11 (64-bit), Administrator privileges recommended for timer & core unparking tweaks.
- **Linux**: Any modern x86_64 distribution with systemd / powerprofilesctl / GameMode support.
