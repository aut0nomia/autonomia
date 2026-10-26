# Autonomia

Modern C++20 project initialized with `make`.

## Requirements

- C++20 compliant compiler (GCC 10+, Clang 12+, etc.)
- GNU Make 4+
- `pkg-config` (optional, for external packages)

## Project Layout

```
.
├── Makefile              # Build system with dependency tracking and debug/release modes
├── include/              # Public header files
│   └── autonomia/
│       └── app.hpp
├── src/                  # Source files
│   ├── app.cpp
│   └── main.cpp
├── bin/                  # Executable binaries (generated)
└── build/                # Object files and dependency metadata (generated)
```

## Quick Start

### Build & Run
```bash
make run
```

### Build Only (Debug by default)
```bash
make
```

### Build & Run with Release Optimizations
```bash
make release
make run
```

### Pass CLI Arguments
```bash
make run ARGS="MyCustomAppName"
```

### Clean
```bash
make clean
```
