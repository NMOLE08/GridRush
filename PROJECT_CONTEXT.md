# Gridrush - Project Context

## Project Overview

**Gridrush** is a generalized Sudoku Constraint Satisfaction Problem (CSP) solver with support for multiple Sudoku variants. It combines a high-performance C++ backend with a web-based frontend for interactive puzzle solving and analysis.

- **Project Name**: gridrush
- **Version**: 1.0.0
- **Type**: Full-stack web application with C++ solver
- **Primary Language**: C++ (backend solver) + JavaScript (frontend/server)
- **Build System**: CMake

## Tech Stack

### Backend
- **Language**: C++17
- **Framework**: Custom CSP solver
- **Build**: CMake 3.16+
- **Key Libraries**: 
  - `nlohmann/json` (JSON parsing)

### Frontend
- **Type**: Static HTML/CSS/JavaScript
- **Files**: 
  - `frontend/index.html` - HTML structure
  - `frontend/app.js` - Main application logic
  - `frontend/styles.css` - Styling

### Server/API Bridge
- **Runtime**: Node.js
- **Framework**: Express.js
- **Port**: 3000
- **Dependencies**: 
  - `express` - Web server
  - `cors` - Cross-origin resource sharing

## Project Structure

```
/
├── CMakeLists.txt              # C++ build configuration
├── package.json                # Node.js dependencies
├── README.md                   # Project documentation
├── build/                      # Build output directory
│   ├── sudoku_solver           # Compiled C++ executable
│   ├── Makefile
│   └── CMakeFiles/
├── src/                        # C++ source files
│   ├── main.cpp               # Entry point for CLI solver
│   └── bitwise_solver.cpp     # Core solving algorithm
├── include/                    # C++ header files
│   ├── bitwise_solver.h       # Solver interface
│   ├── constraints.h          # Constraint definitions
├── frontend/                   # Web UI
│   ├── index.html
│   ├── app.js
│   └── styles.css
└── server/                     # Express server
    └── index.js
```

## Key Features

### Sudoku Variants Supported
- **Classic Sudoku** - Standard 9x9 grid with row/column/box constraints
- **Even-Odd** - Cells marked as even or odd
- **Killer Sudoku** - Cage sums with no repeat constraint
- **Thermos** - Strictly increasing sequences
- **Arrow Sudoku** - Circle with path constraints
- **Kropki Dots** - Consecutive/ratio constraints between cells
- **Hybrid** - Any combination of above constraints

### Core Functionality
1. **Bitwise Constraint Solver** - Efficient domain representation using bitsets
2. **AC-3 Algorithm** - Arc consistency for constraint propagation
3. **Step-by-step Deduction Log** - "Glassbox" logging of solving process
4. **Uniqueness Verification** - Validates puzzle has single solution
5. **JSON Input/Output** - Standardized constraint format

## JSON Schema (Input Format)

Supported constraint fields in puzzle JSON:

| Field | Type | Example |
|-------|------|---------|
| `givens_grid` | 9x9 matrix | `0..9` (0 = empty) |
| `givens` | List of objects | `{"cell": [r,c], "value": d}` (1-based) |
| `even_cells` | List | `[[r,c], ...]` |
| `odd_cells` | List | `[[r,c], ...]` |
| `killer_cages` | List | `{"sum": int, "cells": [[r,c], ...]}` |
| `thermos` | List | `{"cells": [[r,c], ...]}` (bulb first) |
| `arrows` | List | `{"circle": [r,c], "path": [[r,c], ...]}` |
| `kropki` | List | `{"a": [r,c], "b": [r,c], "type": "white\|black"}` |

## Build Instructions

### Prerequisites
- C++17 compatible compiler (Clang/GCC/MSVC)
- CMake 3.16+
- nlohmann/json (header-only library)
- Node.js 14+ (for web server)

### Build C++ Solver
```bash
cmake -S . -B build
cmake --build build
```

### Install Dependencies
```bash
npm install
```

### Start Web Application
```bash
npm start
```
Then open `http://localhost:3000` in browser

## Running

### CLI Solver Only
```bash
./build/sudoku_solver puzzle.json
```

### Web Application Flow
1. Navigate to `http://localhost:3000`
2. Select puzzle category (Classic/Even-Odd/Killer/Thermo/Arrow/Kropki/Hybrid)
3. Enter given digits in the Sudoku grid
4. Adjust variant-specific constraint JSON
5. Click **Start Solver**
6. View:
   - Solved grid
   - Uniqueness verification
   - Step-by-step deduction log

## Core Files

### C++ Implementation
- [src/main.cpp](src/main.cpp) - CLI entry point and JSON file handling
- [src/bitwise_solver.cpp](src/bitwise_solver.cpp) - Core solving algorithm
- [include/bitwise_solver.h](include/bitwise_solver.h) - Solver interface
- [include/constraints.h](include/constraints.h) - Constraint types and validation

### Frontend
- [frontend/index.html](frontend/index.html) - Page structure and UI components
- [frontend/app.js](frontend/app.js) - Client-side logic and API communication
- [frontend/styles.css](frontend/styles.css) - Styling

### Server
- [server/index.js](server/index.js) - Express server and solver bridge

## Development Notes

- The solver uses a bitwise representation of cell domains for efficiency
- AC-3 algorithm ensures arc consistency before backtracking
- Glassbox logging provides detailed insight into deduction steps
- All constraints can be mixed and combined simultaneously
- Coordinates are 1-based in JSON input/output but 0-based internally

## Dependencies

### Runtime
- `express@^4.21.2` - Web server framework
- `cors@^2.8.5` - CORS middleware

### Build-time
- `nlohmann_json` - JSON parsing library (header-only, must be in compiler include path)

## Port Configuration
- **Web Application**: `3000` (configurable in server/index.js)
