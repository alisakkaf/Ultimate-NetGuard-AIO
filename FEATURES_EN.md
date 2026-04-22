# Ultimate NetGuard AIO — Complete Features List

> **Version:** V1.0.0  
> **Author:** Ali Sakkaf  
> **Website:** [alisakkaf.com](https://alisakkaf.com)  
> **GitHub:** [github.com/alisakkaf](https://github.com/alisakkaf)

---

## 🌐 Module 1: Real-Time Network Traffic Monitor

### Core Capture Engine
- **Raw Socket Packet Capture** — Uses Windows `SIO_RCVALL` raw sockets for true packet-level monitoring
- **Dual-Stack IPv4/IPv6** — Simultaneous capture on both IPv4 and IPv6 sockets
- **Automatic Adapter Detection** — Smart algorithm queries Windows routing table (`GetBestInterface`) to auto-select the active internet-connected adapter
- **Live Adapter Auto-Refresh** — Clicking the adapter ComboBox triggers an instant refresh to detect new Wi-Fi/Ethernet/VPN connections on the fly
- **Virtual Adapter Filtering** — Automatically excludes VMware, VirtualBox, Radmin, ZeroTier, and other virtual NICs
- **High-Performance Threading** — Dedicated capture thread with `QThread::HighPriority` and 32MB receive buffer
- **Batch Processing** — Packets are batched (250ms or 3000 packets) to minimize UI overhead
- **Force-Stop Capture** — Graceful stop with 2-second timeout, falls back to forced thread termination if unresponsive
- **Silent Socket Error Recovery** — Automatically restarts the application if a socket bind error occurs

### Per-Process Traffic Tracking
- **Live PID Resolution** — Maps every packet to its owning process using `GetExtendedTcpTable` / `GetExtendedUdpTable`
- **IPv6 Process Mapping** — Full IPv6 TCP/UDP table support with `MIB_TCP6TABLE_OWNER_PID`
- **Service Name Resolution** — Identifies Windows services running under `svchost.exe` using `EnumServicesStatusEx`
- **TCP State-Aware Caching** — Handles LISTEN, ESTABLISHED, and wildcard (0.0.0.0) port bindings
- **Background Cache Thread** — Separate thread refreshes process/connection caches every 1.5 seconds

### Network Tree View
- **Custom QAbstractItemModel** — Hierarchical tree model with process nodes → connection children
- **8 Data Columns** — Application/Protocol, Source, Destination, Service, Download Speed, Upload Speed, Total Bytes, Packets
- **Real-Time Speed Calculation** — Per-connection and per-process speed updated every 1 second
- **Live Sorting** — Custom `UserRole+5` for precise numeric sorting via `QSortFilterProxyModel`
- **Search/Filter** — Real-time recursive filtering across all columns
- **Expand/Collapse All** — Toggle button for tree expansion
- **Process Icons** — Extracts real application icons via `SHGetFileInfoW` + `QtWin::fromHICON`
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

### Shortcut Resolution
- **.lnk File Support** — Resolves Windows shortcuts to real executable paths via `IShellLinkW`
- **Environment Variable Expansion** — Handles `%SystemRoot%`, `%ProgramFiles%`, etc. via `ExpandEnvironmentStringsW`
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
- **Drop EXE Files** — Drag any .exe or .lnk onto the firewall tab to block/allow
- **Action Dialog** — Choose Block, Allow, or Cancel on drop

### Import / Export
- **JSON Export** — Export all NetGuard rules to JSON file
- **JSON Import** — Import and recreate rules from JSON backup

### Pre-Built Firewall Rule Profiles (6 JSON Files)
- **🏢 Global Workspace Shield** — Office environments: browsers, video conferencing, cloud storage, project management tools allowed; unauthorized background apps blocked
- **🚀 Ultimate Esports Nexus** — Pro gamers: ultra-low ping by blocking updates; game launchers, voice chat, streaming tools, anti-cheat engines allowed
- **💻 Master Developer Sandbox** — Software engineers: unrestricted IDEs (Qt Creator, VS Code, Visual Studio, JetBrains), Docker, databases, AI coding assistants
- **🔒 ZeroTrust Privacy Citadel** — Maximum anti-tracking: only privacy browsers (Tor, Brave), VPNs, E2E encrypted messaging allowed
- **📥 P2P Media Vanguard** — Download stations: only download managers (IDM), torrent clients, streaming media players allowed
- **🚫 Offline Isolation Blacklist** — Forces Adobe, Autodesk, Corel suites into offline mode to preserve local licenses and block telemetry

### Firewall Table Model
- **6 Columns** — Application, Rule Name, Action, Protocol, Status, Type
- **Application Icons** — `QFileIconProvider` for real app icons
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
- **JSON Persistence** — Auto-saves to `%AppData%/NetGuard_History.json`
- **Auto-Save** — Writes to disk every 60 seconds during active traffic
- **Save on Exit** — Guaranteed data save in destructor

### History Tree UI
- **4 Time Filters** — Today, Last 7 Days, This Month, All Time
- **Today Mode** — Flat list sorted by total traffic (descending)
- **Multi-Day Mode** — Hierarchical tree grouped by date → applications
- **Summary Cards** — Total Download, Total Upload, Top Application with icon
- **Expand State Preservation** — Remembers which date nodes were expanded during refresh
- **Application Icons** — Cached process icons from live monitor

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
- **Font Size** — 7-16pt adjustable
- **Background Opacity** — 0-255 range
- **Text Color** — White, Black, Green, Blue, Yellow
- **Background Color** — Dark Glass, Solid Black, Solid Blue, Transparent

---

## 🖥️ Module 6: Taskbar Overlay Widget

### Architecture
- **Frameless Transparent Widget** — `Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint`
- **No-Activate** — `WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW` prevents focus stealing
- **Taskbar-Parented** — `SetWindowLongPtr(GWLP_HWNDPARENT)` to `Shell_TrayWnd`
- **Explorer Restart Survival** — Detects parent loss and re-parents automatically
- **Auto-Hide Taskbar Support** — Hides itself when taskbar height < 10px

### Live Speed Display
- **Upload/Download Arrows** — Color-coded (Upload: amber, Download: blue)
- **Auto-Scaling** — B/s → KB/s → MB/s
- **Grid Layout** — Precise icon + value alignment

### Stats Popup (Hover)
- **6 Live Metrics** — Download/Upload speed, Session Total DL/UL, CPU Load, RAM Load
- **Theme-Aware** — Dark tooltip-style popup
- **No-Focus** — `Qt::ToolTip` window flags prevent activation

### Right-Click Menu
- **Increase/Decrease Size** — Dynamic font scaling
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
| OS APIs | WinSock2, IPHelper, WMI/COM, PDH, Shell32, PSAPI |
| Build System | qmake |
| Packet Parsing | Custom wire-format structs (IPv4/IPv6/TCP/UDP/ICMP) |
| Data Model | QAbstractItemModel (custom tree) |
| Persistence | JSON (history), Windows Registry (settings) |
| Theming | QSS (Dark/Light) with StyleManager singleton |
