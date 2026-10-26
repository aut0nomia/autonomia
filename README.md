# Autonomia

Modern C++20 project configured with `make` and `clangd` LSP integration.

---

## Requirements

- **Compiler**: C++20 compliant compiler (`g++` 11+ or `clang++` 13+)
- **Build Tool**: `make`
- **Optional**: [`bear`](https://github.com/rizsotto/Bear) (for generating `compile_commands.json` for LSP servers)

---

## Quick Start

### 1. Build

Compile the project binary:

```sh
make
```

The compiled binary will be placed at `build/app`.

### 2. Run

Run the application directly or via `make`:

```sh
make run
# or run the executable directly:
./build/app
```

### 3. Run Tests

Compile and execute the test runner:

```sh
make test
# or run the test runner directly after building:
./build/test_runner
```

---

## Development & Tooling

### Clean Build Artifacts

Remove all generated build artifacts:

```sh
make clean
```

### Language Server (LSP) Support

The repository includes a [`.clangd`](.clangd) configuration file set up for `clangd` diagnostics, inlay hints, and `clang-tidy` checks.

To regenerate `compile_commands.json` after adding new source files:

```sh
make compdb
```

> **Note**: Requires `bear` to be installed (`sudo dnf install bear` / `sudo apt install bear`).

---

## Project Structure

```text
autonomia/
├── include/              # Header files (.hpp)
│   └── task.hpp
├── src/                  # Application source files (.cpp)
│   └── main.cpp
├── tests/                # Test source files (.cpp)
│   └── test_main.cpp
├── .clangd               # clangd LSP configuration
├── compile_commands.json # Clang compilation database
├── Makefile              # Build automation script
└── README.md
```
