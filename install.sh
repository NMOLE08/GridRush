#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

if ! command -v cmake >/dev/null 2>&1; then
  echo "ERROR: cmake is not installed or not in PATH."
  exit 1
fi

BUILD_DIR="build"
case "${OSTYPE:-}" in
  msys*|cygwin*|win32*) BUILD_DIR="build-win" ;;
esac

cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR" --config Release

echo "Build completed successfully."
