# Ultimate NetGuard AIO — Complete Features List

> **Version:** V1.2.0  
> **Author:** Ali Sakkaf  
> **Website:** [alisakkaf.com](https://alisakkaf.com)  
> **GitHub:** [github.com/alisakkaf](https://github.com/alisakkaf)

---

## 🌐 Module 1: Real-Time Network Traffic Monitor

### Core Capture Engine
- **Raw Socket Packet Capture** — Uses Windows `SIO_RCVALL` raw sockets for true packet-level monitoring with zero WinPcap or Npcap dependencies
- **Dual-Stack IPv4/IPv6** — Simultaneous capture on both IPv4 and IPv6 sockets
- **Smart Auto-Detect Network Adapter Engine** — Advanced algorithm evaluates active traffic bytes (`InOctets + OutOctets`) via `GetIfEntry2` and queries active routing tables (`GetBestInterface` for IPv4 & `GetBestRoute2` for IPv6) to auto-select the physical internet-connected adapter on startup and dynamic network changes
- **Live Adapter Auto-Refresh** — Clicking the adapter ComboBox triggers an instant refresh to detect new Wi-Fi/Ethernet/VPN connections on the fly
- **Virtual Adapter Filtering** — Automatically excludes virtual NICs and VPN interfaces (`VMware`, `VirtualBox`, `Radmin`, `ZeroTier`, `Hamachi`, `TAP/TUN`, `WSL`, `vEthernet`)
- **High-Performance Threading** — Dedicated capture thread with `QThread::HighPriority` and 32MB receive buffer
- **Batch Processing** — Packets are batched (250ms or 3000 packets) to minimize UI overhead
- **Force-Stop Capture** — Graceful stop with 2-second timeout, falls back to forced thread termination if unresponsive
- **Silent Socket Error Recovery** — Automatically restarts the application if a socket bind error occurs

### Per-Process Traffic Tracking & Speed Direction
- **Live PID Resolution** — Maps every packet to its owning process using `GetExtendedTcpTable` / `GetExtendedUdpTable`
- **100% Upload/Download Classification Precision** — Advanced direction handling eliminates Upload vs Download mix-ups under heavy packet bursts, VPNs, and local socket loopbacks
- **IPv6 Process Mapping** — Full IPv6 TCP/UDP table support with `MIB_TCP6TABLE_OWNER_PID`
- **Svchost Multi-Service Resolver** — Resolves and displays all active Windows service display names running under the same `svchost.exe` PID (joined with `+`, e.g., `Windows Update + Background Intelligent Transfer Service`)
- **TCP State-Aware Caching** — Handles LISTEN, ESTABLISHED, and wildcard (0.0.0.0) port bindings
- **Background Cache Thread** — Separate thread refreshes process/connection caches every 200ms for instant responsiveness

### Network Tree View
- **Custom QAbstractItemModel** — Hierarchical tree model with process nodes → connection children
- **8 Data Columns** — Application/Protocol, Source, Destination, Service, Download Speed, Upload Speed, Total Bytes, Packets
- **Real-Time Speed & EMA Smoothing** — Per-connection and per-process speed updated every 1 second with Exponential Moving Average (EMA) smoothing
- **Live Sorting** — Custom `UserRole+5` for precise numeric sorting via `QSortFilterProxyModel`
- **Search/Filter** — Real-time recursive filtering across all columns
- **Expand/Collapse All** — Toggle button for tree expansion
- **100% Icon Resolution Engine** — Extracts real application icons via `SHGetFileInfoW` (`SLGP_UNCPRIORITY`), `QtWin::fromHICON`, environment variable expansion (`ExpandEnvironmentStringsW`), and `System32` fallbacks for system services and closed apps
- **Debug Privilege** — Enables `SeDebugPrivilege` for system process icon access

### Context Menu (Right-Click)
- **Kill Process** — Forcefully terminate any process by PID via `TerminateProcess`
- **Process Properties Dialog** — Professional dialog showing:
  - Large 150×150 process icon
  - Process name, PID, type (Application/Service/System)
  - Full file path (selectable text)
  - Live traffic stats: Download/Upload speed, Total consumed, Packet count
  - "Open File Location" button (opens Explorer with file selected)
- **Copy Connection Details** — Copy source:port info to clipboard

### Dashboard Cards
- **Packets Counter** — Total captured packets
- **Download Speed** — Real-time RX speed with auto-scaling (B/s → KB/s → MB/s)
- **Upload Speed** — Real-time TX speed with auto-scaling
- **Hardware Speed** — System-level speed via `GetIfEntry2` (NIC hardware counters)

### Protocol Support
- TCP, UDP, ICMP, ICMPv6, TCP6, UDP6
- Service name mapping: HTTP, HTTPS, DNS, SSH, FTP, SMTP, IMAP, POP3, MySQL, PostgreSQL, Redis, MongoDB, RDP, VNC, NTP, NetBIOS, SMB, DHCP, IKE, OpenVPN, and more

---

## 🛡️ Module 2: Windows Firewall Manager

### COM API Integration
- **Direct INetFwPolicy2** — Native COM interface to Windows Firewall (no netsh wrapper)
- **INetFwRules Collection** — Full CRUD operations on firewall rules
- **Netsh Fallback** — Automatic fallback to `netsh advfirewall` for edge cases

### Rule Management
- **Block Application** — Creates paired Inbound + Outbound BLOCK rules
- **Allow Application** — Creates paired Inbound + Outbound ALLOW rules
- **Remove Rules** — Delete rules by name or application path
- **Toggle Enable/Disable** — Enable or disable any rule without deleting it
- **Bulk Selection** — Multi-select rules with Ctrl+A and Delete key shortcuts
- **Rule Naming** — Consistent `NetGuard_<AppName>_<ACTION>_<DIR>` naming convention

### Shortcut & Icon Resolution
- **.lnk File Support** — Resolves Windows shortcuts to real executable paths (`.exe`) via `IShellLinkW` with `SLGP_UNCPRIORITY`
- **Full Icon Extraction** — Resolves and displays real icons for system rules, environment paths (`%SystemRoot%`), and system binaries
- **Path Sanitization** — Strict native backslash conversion for COM API compatibility

### Whitelist Lockdown Mode
- **Total Network Lockdown** — Blocks ALL outbound traffic across Domain, Private, and Public profiles
- **Aggressive Rule Disabling** — Disables all non-NetGuard outbound ALLOW rules
- **Critical Service Protection** — Automatically whitelists `svchost.exe` and `System` to prevent OS breakage
- **One-Click Toggle** — Enable/disable with warning confirmation dialog

### Dual-Tab Interface
- **NetGuard Core Tab** — Shows only rules created by Ultimate NetGuard AIO
- **System Rules Tab** — Full Windows Firewall rule browser
- **Direction Toggles** — Inbound/Outbound radio buttons on each tab
- **Search/Filter** — Real-time filtering across all columns

### Drag & Drop
- **Drop EXE & LNK Files** — Drag any .exe or .lnk onto the firewall tab to block/allow
- **Action Dialog** — Choose Block, Allow, or Cancel on drop

### Import / Export
- **JSON Export** — Export all NetGuard rules to JSON file
- **JSON Import** — Import and recreate rules from JSON backup

### Pre-Built Firewall Rule Profiles (6 JSON Files)
- **🏢 Global Workspace Shield** — Office environments: browsers, video conferencing, cloud storage allowed; unauthorized background apps blocked
- **🚀 Ultimate Esports Nexus** — Pro gamers: ultra-low ping by blocking updates; game launchers, voice chat, streaming tools allowed
- **💻 Master Developer Sandbox** — Software engineers: unrestricted IDEs (Qt Creator, VS Code, Visual Studio), Docker, databases, AI coding assistants
- **🔒 ZeroTrust Privacy Citadel** — Maximum anti-tracking: only privacy browsers (Tor, Brave), VPNs, E2E encrypted messaging allowed
- **📥 P2P Media Vanguard** — Download stations: only download managers (IDM), torrent clients, streaming media players allowed
- **🚫 Offline Isolation Blacklist** — Forces Adobe, Autodesk, and Corel suites into offline mode to preserve local licenses and block telemetry

### Firewall Table Model
- **6 Columns** — Application, Rule Name, Action, Protocol, Status, Type
- **Application Icons** — `QFileIconProvider` & `SHGetFileInfoW` for real app icons
- **Color Coding** — Green for Allow, Red for Block, Gold for NetGuard rules
- **Theme-Aware** — Colors adapt to Dark/Light mode

---

## 💻 Module 3: Hardware System Monitor

### Data Sources
- **CPU Load** — PDH counter: `\Processor(_Total)\% Processor Time`
- **Disk Activity** — PDH counter: `\PhysicalDisk(_Total)\% Disk Time`
- **RAM Usage** — `GlobalMemoryStatusEx` API (used/total/percentage)
- **GPU Load** — WMI: `Win32_PerfFormattedData_GPUPerformanceCounters_GPUEngine` (3D engine utilization)
- **Network I/O** — `GetIfTable` API (all non-loopback interfaces)

### Multi-GPU Support (DXGI)
- **DirectX DXGI Enumeration** — Uses `CreateDXGIFactory()` and `IDXGIFactory::EnumAdapters()` to detect all installed GPUs by hardware name (e.g., NVIDIA GeForce RTX, Intel UHD Graphics, AMD Radeon)
- **WMI Adapter Filtering** — Filters WMI performance counters by physical adapter index for precise per-GPU monitoring

### Temperature Monitoring
- **CPU Temperature** — Multi-source with cascading fallbacks:
  1. `MSAcpi_ThermalZoneTemperature` (ROOT\WMI) — Kelvin conversion
  2. `Win32_TemperatureProbe` (CIMV2) — Direct Celsius
  3. `Win32_PerfFormattedData_Counters_ThermalZoneInformation` — Hybrid
- **Disk Temperature** — `MSFT_StorageReliabilityCounter` (ROOT\Microsoft\Windows\Storage)
- **GPU Temperature** — WMI GPU sensor queries
- **Motherboard Temperature** — Extracted from ACPI thermal zones (lowest sensor reading)

### WMI Architecture
- **3 WMI Namespaces** — `ROOT\CIMV2`, `ROOT\WMI`, `ROOT\Microsoft\Windows\Storage`
- **Robust Variant Parsing** — Handles VT_I4, VT_UI4, VT_R4, VT_R8, VT_I2, VT_UI2, VT_I8, VT_UI8, VT_BSTR
- **Threaded Collection** — Background `QThread` with 1-second polling interval

### Custom UI Widgets
- **CircularGauge** — Custom `QPaintEvent` circular arc gauge with:
  - Value arc (blue < 60%, amber < 85%, red ≥ 85%)
  - Center percentage text (18pt bold)
  - Temperature sub-text
  - Hardware label
- **TempBar** — Horizontal temperature bar with color-coded fill
- **Theme-Aware Drawing** — Uses `QPalette` for automatic Dark/Light adaptation

---

## 📊 Module 4: Network Usage History

### Smart Data Engine
- **Per-Application Tracking** — Records download/upload bytes per app per day
- **Live Extraction** — Reads data directly from the NetworkTreeModel every second
- **`exePath` Persistence** — Saves full application executable paths in `NetGuard_History.json` to guarantee icon extraction
- **JSON Persistence** — Auto-saves to `%AppData%/NetGuard_History.json`
- **Auto-Save** — Writes to disk every 60 seconds during active traffic
- **Save on Exit** — Guaranteed data save in destructor

### History Tree UI
- **4 Time Filters** — Today, Last 7 Days, This Month, All Time
- **Today Mode** — Flat list sorted by total traffic (descending)
- **Multi-Day Mode** — Hierarchical tree grouped by date → applications
- **Summary Cards** — Total Download, Total Upload, Top Application with icon
- **Expand State Preservation** — Remembers which date nodes were expanded during refresh
- **100% Icon Extraction for Offline Apps** — `getOrResolveIcon` retrieves and displays real icons for closed/offline applications even after system reboot

### Data Management
- **Export to CSV** — Export visible history data with parent/child formatting
- **Clear All** — Delete all history with confirmation dialog
- **Thread-Safe** — Timer-based UI refresh only when History tab is active

---

## ⚙️ Module 5: Settings & Configuration

### General Settings
- **Run at Startup** — Windows Registry auto-start (`HKCU\...\Run`)
- **Start Minimized** — Launch directly to system tray
- **Dark/Light Theme Toggle** — Animated sun/moon button with full QSS theme switching
- **Settings Persistence** — `QSettings` (Windows Registry) for all preferences

### Taskbar Overlay Customization
- **Enable/Disable Overlay** — Toggle the taskbar speed widget
- **Hardware Module Toggles** — Independent toggles for CPU, RAM, GPU, and Temperature display
- **RAM Display Formats** — Select Percentage (%), Megabytes (MB), or Gigabytes (GB)
- **GPU Selection** — Select specific GPU adapter to monitor (Auto or specific DXGI index)
- **Font Size** — 7-16pt adjustable
- **Background Opacity** — 0-255 range
- **Text Color** — White, Black, Green, Blue, Yellow
- **Background Color** — Dark Glass, Solid Black, Solid Blue, Transparent

---

## 🖥️ Module 6: Taskbar Overlay Widget (Taskbar Overlay 2.0)

### Architecture & Always Top-Most Visibility
- **Frameless Transparent Widget** — `Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint`
- **No-Activate** — `WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW` prevents focus stealing
- **Taskbar-Parented** — `SetWindowLongPtr(GWLP_HWNDPARENT)` to `Shell_TrayWnd`
- **Always Top-Most Z-Order** — Enforces `Qt::WindowStaysOnTopHint` and `SetWindowPos(HWND_TOPMOST)` for guaranteed visibility above all windows
- **Explorer Restart Survival** — Detects parent loss and re-parents automatically
- **Auto-Hide Taskbar Support** — Hides itself when taskbar height < 10px

### Free-Form Drag & Drop Placement Anywhere
- **Position Anywhere** — Left-click and drag the overlay anywhere on desktop, screen edges, or taskbar
- **Persistent Position** — Saved across restarts, resolution changes, and DPI scaling
- **One-Click Reset Position** — New `🔄 Reset Position` right-click context menu action restores default taskbar positioning instantly

### Direct Taskbar Adapter Selection
- **`🌐 Select Network Adapter` Submenu** — Embedded directly in right-click context menu; allows switching active adapter or triggering Smart Auto-Detect with 1 click without opening main window

### Live Speed & Hardware Display
- **Upload/Download Arrows** — Color-coded (Upload: amber, Download: blue)
- **Auto-Scaling** — B/s → KB/s → MB/s
- **Hardware Metrics** — Shows CPU %, RAM (GB/MB/%), GPU %, and Temps (°CPU/GPU)
- **Protective Text Shadows** — `QGraphicsDropShadowEffect` ensures 100% legibility over any background

### Stats Popup (Hover)
- **10 Live Metrics** — Download/Upload speed, Session Total DL/UL, CPU Load, RAM Load, GPU Load, GPU RAM Detail, CPU Temp, GPU Temp
- **Theme-Aware** — Dark tooltip-style popup
- **No-Focus** — `Qt::ToolTip` window flags prevent activation

### Right-Click Context Menu
- **Select Network Adapter** — Direct adapter switcher or Smart Auto-Detect
- **Increase/Decrease Size** — Dynamic font scaling
- **Reset Position** — Restore default taskbar position
- **Show/Minimize App** — Window visibility control
- **Exit NetGuard** — Application quit

---

## 🔔 Module 7: System Tray Icon

- **Dynamic Activity Indicator** — Green dot when network traffic detected, gray when idle
- **Real App Icon** — Loads from Qt resources with `HICON` fallback
- **Rich Tooltip** — Shows current download/upload speeds
- **Context Menu** — Show NetGuard / Quit NetGuard
- **Click to Toggle** — Double-click to show/hide main window

---

## 🎨 Module 8: Theme System

### Dark Theme (Default)
- **Zinc Color Palette** — #18181B background, #E4E4E7 text, #3F3F46 borders
- **Blue Accents** — #3B82F6 primary, #60A5FA hover
- **Emerald Accents** — #10B981 for upload/success indicators
- **Full Coverage** — QSS for every Qt widget type

### Light Theme
- **Clean White** — #F4F4F5 background, #27272A text
- **Blue Accents** — #2563EB primary
- **Professional Contrast** — Selection and hover states tuned for readability

### Style Manager
- **Singleton Pattern** — `StyleManager::instance()` global access
- **Signal Emission** — `themeChanged()` signal for reactive UI updates
- **Live Toggle** — Theme switch without restart

---

## 🔄 Module 9: Auto-Updater & Self-Installer

### Smart Self-Installer
- **Auto-Deploy** — On first launch, copies itself to `C:\Program Files\NetGuard\UltimateNetGuard.exe`
- **UAC Elevation** — Automatically requests Administrator via `ShellExecuteExW` (runas)
- **Desktop Shortcut** — Creates `NetGuard AIO.lnk` via `IShellLink` COM interface
- **File Permissions** — Sets strict Read/Write/Execute permissions on the installed binary
- **Version Tracking** — Writes `version.dat` to detect reinstalls vs updates
- **Kill-and-Replace** — On update, terminates old process before overwriting the binary
- **Install Progress Dialog** — Frameless, draggable, theme-aware progress window

### Silent OTA Updater
- **Background Check** — Silently fetches remote version file 2 seconds after startup
- **Semantic Version Comparison** — Intelligent `major.minor.patch` comparison, triggers only when remote is strictly higher
- **Update Dialog** — Frameless, draggable, theme-aware dialog showing new version, changelog, and action buttons
- **Website Redirect** — "Update Now" button opens the official download page on the developer's website

---

## 🔒 Security & System Features

- **100% Clean Codebase** — Zero Arabic comments in C++ source files; standardized English code documentation
- **Administrator Elevation** — UAC manifest with `requireAdministrator`
- **Single Instance (QSharedMemory)** — Qt-native `QSharedMemory` guard prevents multiple running instances; if a second instance launches, it finds and focuses the existing window via `FindWindowW`
- **DPI Unaware** — Consistent layout across all scaling factors (DWM handles bitmap scaling)
- **Windows 7-11 Compatibility** — Full `supportedOS` manifest entries
- **Firewall Auto-Rule** — Automatically creates inbound allow rule for itself
- **Minimize to Tray** — Close button minimizes to system tray (not exit)
- **Custom Windows Notifications** — Tray balloon messages
- **Registry Auto-Startup** — Optional Windows startup via `HKCU\...\Run` registry key

---

## 🏗️ Technical Stack

| Component | Technology |
|-----------|-----------|
| Framework | Qt 5.14.2 (MinGW 32-bit, Static Build) |
| Language | C++14 |
| OS APIs | WinSock2, IPHelper, WMI/COM, DirectX DXGI, PDH, Shell32, PSAPI, QtWin |
| Build System | qmake |
| Packet Parsing | Custom wire-format structs (IPv4/IPv6/TCP/UDP/ICMP) |
| Data Model | QAbstractItemModel (custom tree) |
| Persistence | JSON (history), Windows Registry (settings) |
| Theming | QSS (Dark/Light) with StyleManager singleton |
