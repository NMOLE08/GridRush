# Generalized Sudoku CSP Solver (Bitwise + AC-3 + Glassbox Log)

This project now includes:
- C++ solver backend (`build/sudoku_solver`)
- Node/Express API bridge (`server/index.js`)
- React frontend (`frontend/index.html`, `frontend/app.js`, `frontend/styles.css`)

## Build

Requires C++17 and nlohmann/json single-header package available to compiler include path.

```bash
cmake -S . -B build
cmake --build build
```

## Run (CLI solver only)

```bash
./build/sudoku_solver puzzle.json
```

## JSON Schema (supported)

- `givens_grid`: 9x9 matrix, values `0..9` (`0` = empty)
- `givens`: list of `{ "cell": [r,c], "value": d }` (1-based coordinates)
- `even_cells`: list of `[r,c]`
- `odd_cells`: list of `[r,c]`
- `killer_cages`: list of `{ "sum": int, "cells": [[r,c], ...] }`
- `thermos`: list of `{ "cells": [[r,c], ...] }` (bulb first)
- `arrows`: list of `{ "circle": [r,c], "path": [[r,c], ...] }`
- `kropki`: list of `{ "a": [r,c], "b": [r,c], "type": "white|black" }`

All constraints can be mixed simultaneously.

## Web App (Frontend + Backend)

### 1) Build C++ solver

```bash
cmake -S . -B build
cmake --build build
```

### 2) Install Node dependencies

```bash
npm install
```

### 3) Start web server

```bash
npm start
```

Open http://localhost:3000

### Web flow

1. Select puzzle category (Classic / Even-Odd / Killer / Thermo / Arrow / Kropki / Hybrid).
2. The Sudoku grid appears.
3. Enter given digits in the grid.
4. Adjust variant-constraint JSON for the selected category.
5. Click **Start Solver**.
6. View solved grid + uniqueness + step-by-step glassbox deduction log.
