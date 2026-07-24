<div align="center">

<img src="https://raw.githubusercontent.com/alisakkaf/Ultimate-NetGuard-AIO/main/screenshots/Ultimate_NetGuard_AIO_Logo.png" alt="Ultimate NetGuard AIO" width="120" height="120">

# 🛡️ Ultimate NetGuard AIO — v1.2.0

### ⚡ Smart Auto-Detect Engine · Free-Form Overlay Placement · Direct Taskbar Network Control · 100% Icon Engine

[![Version](https://img.shields.io/badge/Version-1.2.0-2EA043.svg?style=for-the-badge)](https://github.com/alisakkaf/Ultimate-NetGuard-AIO/releases/tag/v1.2.0)
[![Platform](https://img.shields.io/badge/Platform-Windows%207--11-0078D6.svg?style=for-the-badge&logo=windows&logoColor=white)](https://github.com/alisakkaf/Ultimate-NetGuard-AIO)
[![License](https://img.shields.io/badge/License-MIT-green.svg?style=for-the-badge)](LICENSE)
[![Downloads](https://img.shields.io/github/downloads/alisakkaf/Ultimate-NetGuard-AIO/v1.2.0/total?style=for-the-badge&color=10B981&label=v1.2%20Downloads)](https://github.com/alisakkaf/Ultimate-NetGuard-AIO/releases/tag/v1.2.0)

**Smart Auto-Detect Network Adapter · Taskbar Overlay Position Drag & Top-Most · 1-Click Taskbar Adapter Switching · Svchost Multi-Service Resolver · 100% Icon Extraction Engine**

[📥 Download v1.2.0](https://github.com/alisakkaf/Ultimate-NetGuard-AIO/releases/tag/v1.2.0) · [📖 Full README](README.md) · [🌐 Website](https://alisakkaf.com/en/windows-software/ultimate-netguard-aio-network-monitor-firewall-manager) · [🐛 Report Bug](https://github.com/alisakkaf/Ultimate-NetGuard-AIO/issues)

</div>

---

## 📋 Release Summary

**v1.2.0** introduces major innovations for network adapter detection intelligence, taskbar overlay positioning freedom, and deep Windows process/service resolution accuracy.

This release introduces an active **Smart Auto-Detect Engine** that inspects real packet traffic (`InOctets + OutOctets`) and routing tables to automatically select the physical internet adapter while filtering out virtual/VPN NICs, **direct 1-click network switching** from the Taskbar Overlay context menu, **free-form drag positioning anywhere** on screen with Top-Most Z-order and **🔄 Reset Position**, multi-service display for `svchost.exe`, and a **100% icon extraction engine** for shortcuts, system binaries, and closed apps.

---

## ✨ What's New in v1.2.0

### ⚡ Smart Auto-Detect Network Adapter Engine
- **Active Traffic & Routing Inspection**: Evaluates active byte traffic (`InOctets + OutOctets`) via `GetIfEntry2`, queries active routing tables (`GetBestInterface` for IPv4 & `GetBestRoute2` for IPv6).
- **Virtual Adapter Filtering**: Automatically excludes virtual interfaces (`VMware`, `VirtualBox`, `Radmin`, `ZeroTier`, `Hamachi`, `TAP/TUN`, `WSL`, `vEthernet`).
- **Dynamic Adaptation**: Auto-selects active internet adapter on startup and adapts when network interfaces change (Wi-Fi, Ethernet, VPN).
- **Default Selection**: Available as Option #1 (`⚡ Smart Auto-Detect (Auto)`) in dashboard and taskbar dropdowns.

### 🌐 Direct Taskbar Overlay Network Adapter Switcher
- New `🌐 Select Network Adapter` submenu built directly inside the Taskbar Overlay right-click context menu.
- Enables 1-click adapter switching or Smart Auto-Detect triggering directly from the taskbar without opening the main window.

### 🖱️ Free-Form Drag & Drop Positioning Anywhere & Always Top-Most
- Position the Taskbar Overlay **anywhere on desktop, screen edges, or taskbar**.
- Maintains **Top-Most Z-order** (`Qt::WindowStaysOnTopHint`, `HWND_TOPMOST`) above all windowed and full-screen applications.
- Added **🔄 Reset Position** action in context menu to restore default taskbar notification positioning instantly.

### 🧩 Svchost Multi-Service Traffic Resolver
- Resolves and displays all active Windows service display names running under the same `svchost.exe` PID (joined with `+`, e.g., `Windows Update + Background Intelligent Transfer Service`).
- 100% accurate Upload vs Download classification even under heavy packet bursts, VPN tunnels, or local socket loopbacks.

### 🎨 100% Icon Extraction Engine
- Deep icon resolution via `SHGetFileInfoW` (`SLGP_UNCPRIORITY`), `QtWin::fromHICON`, environment variable expansion (`ExpandEnvironmentStringsW`), and `System32` fallbacks.
- Correctly extracts real icons for `.lnk` shortcuts, system services, offline applications, and closed processes across Network Traffic, Firewall, and Bandwidth History tables.

### 🧹 Clean Codebase
- 0% Arabic comments in C++ source files (100% standardized English code documentation).

---

## 📊 v1.1.0 vs v1.2.0

| Feature | v1.1.0 | v1.2.0 |
|---------|:------:|:------:|
| Network Traffic Monitor | ✅ | ✅ |
| Windows Firewall Manager | ✅ | ✅ |
| Hardware Monitor (Gauges Tab) | ✅ | ✅ |
| Usage History & CSV Export | ✅ | ✅ |
| Taskbar Overlay Hardware Dashboard | ✅ | ✅ |
| Smart Auto-Detect Network Adapter | ❌ | ✅ |
| 1-Click Taskbar Adapter Switching | ❌ | ✅ |
| Free-Form Overlay Placement Anywhere | ❌ | ✅ |
| Top-Most Overlay Z-Order | ❌ | ✅ |
| 🔄 Reset Position Context Menu | ❌ | ✅ |
| Svchost Multi-Service Resolver | ❌ | ✅ |
| 100% Icon Engine (.lnk / closed apps / System32) | ❌ | ✅ |
| Pre-Built Firewall Rule Profiles | ✅ | ✅ |
| 100% English Source Code | ❌ | ✅ |

---

<div align="center">

# 🛡️ Ultimate NetGuard AIO — v1.1.0

### 🚀 Taskbar Overlay 2.0 — Complete Overhaul

[![Version](https://img.shields.io/badge/Version-1.1.0-2EA043.svg?style=for-the-badge)](https://github.com/alisakkaf/Ultimate-NetGuard-AIO/releases/tag/v1.1.0)
[![Platform](https://img.shields.io/badge/Platform-Windows%207--11-0078D6.svg?style=for-the-badge&logo=windows&logoColor=white)](https://github.com/alisakkaf/Ultimate-NetGuard-AIO)
[![License](https://img.shields.io/badge/License-MIT-green.svg?style=for-the-badge)](LICENSE)
[![Downloads](https://img.shields.io/github/downloads/alisakkaf/Ultimate-NetGuard-AIO/v1.1.0/total?style=for-the-badge&color=10B981&label=v1.1%20Downloads)](https://github.com/alisakkaf/Ultimate-NetGuard-AIO/releases/tag/v1.1.0)

**Real-Time Hardware Intelligence · Multi-GPU Detection · Smart Auto-Layout · Drag & Drop Positioning**

[📥 Download v1.1.0](https://github.com/alisakkaf/Ultimate-NetGuard-AIO/releases/tag/v1.1.0) · [📖 Full README](README.md) · [🌐 Website](https://alisakkaf.com/en/windows-software/ultimate-netguard-aio-network-monitor-firewall-manager) · [🐛 Report Bug](https://github.com/alisakkaf/Ultimate-NetGuard-AIO/issues)

</div>

---

## 📋 Release Summary (v1.1.0)

**v1.1.0** delivers a **ground-up overhaul** of the Taskbar Overlay system, transforming it from a simple network speed indicator into a **full-featured, real-time hardware dashboard** that lives seamlessly on your Windows taskbar.

This release introduces **live CPU, RAM, GPU, and Temperature monitoring** directly on the taskbar overlay, with intelligent multi-GPU detection via DXGI that identifies every graphics adapter installed in the system by its real hardware name.

---

## ✨ What's New in v1.1.0

### 🖥️ Real-Time Hardware Monitoring on Taskbar

Four new independent hardware monitoring modules on the taskbar overlay:

| Module | Description |
|--------|-------------|
| **CPU Usage** | Real-time processor utilization percentage via Windows PDH |
| **RAM Usage** | Live memory consumption with 3 configurable display formats (%, MB, GB) |
| **GPU Usage** | Per-GPU engine utilization via WMI with multi-adapter filtering |
| **Temperature** | Combined CPU°/GPU° temperature display (e.g., `72/65°`) |

Each module can be independently toggled on/off from **Settings → Taskbar Overlay Customization**.

### 🔍 Multi-GPU Auto-Detection & Selection

- Automatically detects **all installed GPUs** by their real hardware name using DXGI
- Choose between `Auto (All GPUs Combined)` or monitor a **specific GPU** individually
  - Example: `Intel(R) UHD Graphics 630`, `NVIDIA GeForce RTX 3080`
- Perfect for systems with integrated + dedicated GPUs

### 📊 Flexible RAM Display Formats

| Mode | Example Output | Best For |
|------|---------------|----------|
| **Percentage** | `65%` | Quick glance |
| **Megabytes** | `12045/16384 MB` | Precise tracking |
| **Gigabytes** | `12/16 GB` | Balanced clarity (default) |

Changing format instantly updates the overlay — no delay.

### 🖱️ Smart Drag & Drop Positioning

- **Drag** the overlay to any position on the taskbar
- Position **persists** across restarts, taskbar resizes, resolution & DPI changes
- **🔄 Reset Position** action in right-click context menu

### 💎 UI/UX Improvements

- **Text Shadows** on all overlay labels — 100% readable over any taskbar background
- **Smart Auto-Resizing** — overlay perfectly hugs its content, zero dead space
- **Enhanced Hover Popup** — now shows 10 live metrics across 5 rows:

| Row | Left | Right |
|-----|------|-------|
| 1 | ↓ Download Speed | ↑ Upload Speed |
| 2 | 📥 Total Download | 📤 Total Upload |
| 3 | 💻 CPU Load | 🧠 RAM Load |
| 4 | 🎮 GPU Load | 💾 RAM Detail |
| 5 | 🌡 CPU Temp | 🌡 GPU Temp |

---

## 🐛 Bug Fixes

| Issue | Fix |
|-------|-----|
| Context menu rendering off-screen | Now positions correctly above the overlay widget |
| Taskbar icon collision on full taskbars | Dynamic shrink-to-fit layout + drag offset system |
| GPU load over-reporting | Correctly filters by selected adapter instead of summing all |
| RAM label text jitter | Auto-resizes after every update for stable display |

---

## 📊 Version History

| Version | Date | Highlights |
|---------|------|------------|
| **v1.2.0** | July 2026 | Smart Auto-Detect Adapter, Free Positioning Anywhere, Top-Most, 1-Click Taskbar Switcher, Svchost Resolver, 100% Icon Engine |
| **v1.1.0** | May 2026 | Taskbar Overlay 2.0: Hardware monitoring, Multi-GPU, Drag & Drop, RAM formats |
| **v1.0.0** | April 2026 | Initial release: Network Monitor, Firewall Manager, Hardware Monitor, Usage History |

---

<div align="center">

**Full Changelog**: [v1.1.0...v1.2.0](https://github.com/alisakkaf/Ultimate-NetGuard-AIO/compare/v1.1.0...v1.2.0)

---

**Developed with ❤️ by [Ali Sakkaf](https://github.com/alisakkaf)**

[![Website](https://img.shields.io/badge/Website-alisakkaf.com-2563EB?style=for-the-badge&logo=google-chrome&logoColor=white)](https://alisakkaf.com)
[![Facebook](https://img.shields.io/badge/Facebook-AliSakkaf.Dev-1877F2?style=for-the-badge&logo=facebook&logoColor=white)](https://www.facebook.com/AliSakkaf.Dev/)
[![GitHub](https://img.shields.io/badge/GitHub-alisakkaf-181717?style=for-the-badge&logo=github&logoColor=white)](https://github.com/alisakkaf)

⭐ **If you like this project, please give it a star!** ⭐

**© 2026 Ali Sakkaf. All Rights Reserved.**

</div>
