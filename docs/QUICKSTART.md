# GridRush Quickstart (Windows + macOS)

This is the fastest path to run the app from a fresh clone.

## Required After Clone

Before running `npm start`, you must build the C++ solver backend at least once.
If you skip this step, the app will show:

`Solver binary not found. Build backend first with cmake -S . -B build && cmake --build build`

Run this command from project root:

```bash
cmake -S . -B build && cmake --build build
```

## 1) Prerequisites

Install these first:

1. Node.js 18+ (includes npm)
2. CMake 3.16+
3. C++17 compiler toolchain

Windows:

1. Visual Studio Build Tools (Desktop C++) or LLVM/Clang toolchain

macOS:

1. Xcode Command Line Tools (`xcode-select --install`)

## 2) Install Node dependencies

From project root:

```bash
npm install
```

## 3) Build solver

### Windows

```powershell
cmake -S . -B build-win
cmake --build build-win
```

### macOS

```bash
cmake -S . -B build
cmake --build build
```

Quick fix command (works on both Windows and macOS from project root):

```bash
cmake -S . -B build
cmake --build build
```

## 4) Start server

```bash
npm start
```

Open:

1. http://localhost:3000

## 5) Verify solver is detected

Open:

1. http://localhost:3000/api/health

Expected:

1. `solverFound: true`

If `solverFound` is false, rebuild solver in the correct folder:

1. Windows: [build-win](../build-win)
2. macOS: [build](../build)

## 6) Use the app

1. Choose category
2. Enter puzzle givens
3. Add/edit variant constraints with tools and JSON
4. Click Start Solver
5. Optional: Check Uniqueness

## Common fixes

1. Port busy: run with a different port (`PORT=3001 npm start`)
2. Solver timeout: increase `SOLVER_TIMEOUT_MS` or add more constraints
3. Solver missing: rebuild using step 3 and retry health endpoint
