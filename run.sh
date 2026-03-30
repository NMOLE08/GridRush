#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

find_solver_bin() {
  local candidates=(
    "build/sudoku_solver"
    "build/Release/sudoku_solver"
    "build/sudoku_solver.exe"
    "build/Release/sudoku_solver.exe"
    "build-win/Release/sudoku_solver.exe"
    "build-win/sudoku_solver.exe"
  )

  local c
  for c in "${candidates[@]}"; do
    if [[ -f "$c" ]]; then
      echo "$c"
      return 0
    fi
  done
  return 1
}

BIN="$(find_solver_bin || true)"

if [[ -z "$BIN" ]]; then
  if [[ -x "install.sh" ]]; then
    ./install.sh
  else
    bash install.sh
  fi

  BIN="$(find_solver_bin || true)"
fi

if [[ -z "$BIN" ]]; then
  echo "ERROR: Solver binary not found after build."
  exit 1
fi

INPUT="questions.json"
if [[ ! -f "$INPUT" ]]; then
  INPUT="question.json"
fi

if [[ ! -f "$INPUT" ]]; then
  echo "ERROR: Input file not found. Expected questions.json or question.json in repository root."
  exit 1
fi

"$BIN" "$INPUT" "answer.json"

if [[ ! -f "answer.json" ]]; then
  echo "ERROR: answer.json was not generated."
  exit 1
fi

echo "Run completed successfully."
echo "Input : $INPUT"
echo "Output: answer.json"
