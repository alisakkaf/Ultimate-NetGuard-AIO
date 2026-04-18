# Contributing to Ultimate NetGuard AIO

Thank you for your interest in contributing to **Ultimate NetGuard AIO**! This document provides guidelines for contributing to the project.

## How to Contribute

### Reporting Bugs

1. Check the [existing issues](https://github.com/alisakkaf/Ultimate-NetGuard-AIO/issues) to avoid duplicates
2. Use the **Bug Report** issue template
3. Include:
   - Windows version (7, 8.1, 10, 11) and build number
   - Steps to reproduce
   - Expected behavior vs. actual behavior
   - Screenshots if applicable
   - Console/debug output if available

### Suggesting Features

1. Open a [Feature Request](https://github.com/alisakkaf/Ultimate-NetGuard-AIO/issues/new) issue
2. Describe the feature and why it would be useful
3. Include mockups or examples if possible

### Pull Requests

1. Fork the repository
2. Create a feature branch from `main`:
   ```bash
   git checkout -b feature/your-feature-name
   ```
3. Follow the coding standards below
4. Test your changes on Windows 10/11
5. Commit with clear messages:
   ```bash
   git commit -m "feat: add XYZ support to network monitor"
   ```
6. Push and open a Pull Request

## Coding Standards

### C++ Style
- **C++14** standard
- **4-space indentation** (no tabs)
- **Allman brace style** for classes, **K&R** for functions
- **camelCase** for variables and functions
- **PascalCase** for class names
- **m_** prefix for member variables
- **ALL_CAPS** for macros and constants

### Qt Conventions
- Use `Q_OBJECT` macro for all QObject-derived classes
- Use new-style signal/slot connections: `connect(obj, &Class::signal, ...)`
- Use `QStringLiteral()` for compile-time string optimization where appropriate
- Always check return values of COM API calls

### Commit Messages
Follow [Conventional Commits](https://www.conventionalcommits.org/):
- `feat:` — New feature
- `fix:` — Bug fix
- `docs:` — Documentation changes
- `style:` — Code style (no logic change)
- `refactor:` — Code refactoring
- `perf:` — Performance improvement
- `test:` — Adding tests

## Development Setup

### Prerequisites
- Qt 5.14.2 with MinGW 32-bit
- Git
- Qt Creator (recommended) or VS Code with C++ extensions

### Build
```bash
git clone https://github.com/alisakkaf/Ultimate-NetGuard-AIO.git
cd Ultimate-NetGuard-AIO
qmake UltimateNetGuard.pro CONFIG+=debug
mingw32-make -j4
```

### Testing
- Run the application as Administrator
- Test on both Windows 10 and Windows 11
- Verify Dark and Light themes
- Test with multiple active network adapters
- Test with VPN connections active

## Code of Conduct

This project follows the [Contributor Covenant Code of Conduct](CODE_OF_CONDUCT.md). By participating, you agree to uphold this code.

---

**Thank you for contributing! ❤️**
