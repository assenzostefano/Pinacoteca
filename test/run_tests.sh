#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

WITH_ARDUINO=false
CLEAN=false

for arg in "$@"; do
  case "$arg" in
    --with-arduino)
      WITH_ARDUINO=true
      ;;
    --clean)
      CLEAN=true
      ;;
    *)
      echo "Argomento non riconosciuto: $arg"
      echo "Uso: ./run_tests.sh [--clean] [--with-arduino]"
      exit 1
      ;;
  esac
done

if $CLEAN; then
  rm -rf "$BUILD_DIR"
fi

cmake -S "$SCRIPT_DIR" -B "$BUILD_DIR"
cmake --build "$BUILD_DIR" -j
ctest --test-dir "$BUILD_DIR" --output-on-failure

if $WITH_ARDUINO; then
  arduino-cli compile -b arduino:avr:uno --output-dir "$PROJECT_ROOT/build" "$PROJECT_ROOT/Pinacoteca.ino"
fi

echo "Tutti i test completati con successo."
