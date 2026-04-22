# Security Policy

## Supported Versions

| Version | Supported          |
|---------|--------------------|
| 1.0.x   | ✅ Currently supported |
| < 1.0   | ❌ No longer supported |

## Reporting a Vulnerability

If you discover a security vulnerability in Ultimate NetGuard AIO, please report it responsibly.

### How to Report

1. **DO NOT** create a public GitHub issue for security vulnerabilities
2. Send an email to the developer via the contact form on [alisakkaf.com](https://alisakkaf.com)
3. Include the following details:
   - Description of the vulnerability
   - Steps to reproduce
   - Potential impact
   - Suggested fix (if any)

### What to Expect

- **Acknowledgment**: Within 48 hours of your report
- **Assessment**: Within 7 days, we will assess the severity
- **Fix**: Critical vulnerabilities will be patched within 14 days
- **Disclosure**: We will coordinate with you on public disclosure timing

### Scope

The following are considered in scope:
- Buffer overflow vulnerabilities in packet parsing
- Privilege escalation beyond intended admin scope
- Data exposure through history files
- COM API misuse leading to system instability
- Firewall rule injection bypassing user confirmation

The following are NOT in scope:
- Issues requiring physical access to the machine
- Social engineering attacks against users
- Issues in third-party libraries or Qt framework itself
- Windows OS vulnerabilities

## Security Best Practices

### For Users
- Always download from the [official GitHub releases](https://github.com/alisakkaf/Ultimate-NetGuard-AIO/releases)
- Verify the file hash against the published checksums
- Keep your Windows OS updated
- Review firewall rules created by the application periodically
- Use Whitelist Lockdown Mode with caution — it blocks ALL outbound traffic

### For Developers
- All COM pointers must be properly released (RAII pattern)
- All BSTR allocations must be freed with `SysFreeString()`
- Buffer sizes for `recv()` and WMI queries must be bounds-checked
- User input (file paths, rule names) must be sanitized before COM API calls
- The `expandAndCleanPath()` function must always be used before passing paths to `INetFwRules`

## Software Bill of Materials (SBOM)

| Component | Version | License | Purpose |
|-----------|---------|---------|---------|
| Qt Framework | 5.14.2 | LGPL v3 / Commercial | UI framework, event loop, threading |
| MinGW | 7.3.0 | GPL v3 (compiler only) | C++ compiler (not linked into binary) |
| Windows SDK | 10.0+ | Microsoft EULA | Headers for Windows APIs |

> **Note:** When built as a static Qt binary, no external DLLs are required at runtime. The application is entirely self-contained.

## Antivirus False Positives

This application may trigger false positives in some antivirus software due to:

1. **Raw socket usage** (`SIO_RCVALL`) — This API is legitimately used for network monitoring but can also be used by malicious software
2. **Firewall rule modification** — The COM API calls to `INetFwPolicy2` may trigger behavior-based detection
3. **Process enumeration** — `EnumProcesses`, `OpenProcess`, and `TerminateProcess` calls are flagged by some heuristics
4. **Auto-start registry modification** — Writing to `HKCU\Software\Microsoft\Windows\CurrentVersion\Run`
5. **Taskbar injection** — Using `SetWindowLongPtr(GWLP_HWNDPARENT)` to parent a window to the taskbar

### Recommended Actions
- Build from source to verify the code yourself
- Submit the binary to your AV vendor for whitelisting
- Add an exception for `UltimateNetGuard.exe` in your antivirus settings
- Check VirusTotal results linked in the release notes
