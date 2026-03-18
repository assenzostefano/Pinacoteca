#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

WITH_ARDUINO=false
CLEAN=false
UPLOAD=false
BOARD_FQBN="arduino:renesas_uno:unor4wifi"
PORT=""

print_help() {
  cat <<EOF
Uso: ./run_tests.sh [opzioni]

Opzioni:
  --clean                 Pulisce la build dei test
  --with-arduino          Compila lo sketch Arduino dopo i test unitari
  --upload                Carica lo sketch sulla board (implica --with-arduino)
  --board <fqbn>          Imposta la board FQBN (default: arduino:renesas_uno:unor4wifi)
  --port <serial_port>    Porta seriale per upload (es: /dev/ttyACM0)
  --help                  Mostra questo help

Esempi:
  ./run_tests.sh --clean
  ./run_tests.sh --with-arduino --board arduino:renesas_uno:unor4wifi
  ./run_tests.sh --upload --board arduino:renesas_uno:unor4wifi --port /dev/ttyACM0
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --with-arduino)
      WITH_ARDUINO=true
      shift
      ;;
    --clean)
      CLEAN=true
      shift
      ;;
    --upload)
      UPLOAD=true
      WITH_ARDUINO=true
      shift
      ;;
    --board)
      if [[ $# -lt 2 ]]; then
        echo "Errore: --board richiede un valore"
        exit 1
      fi
      BOARD_FQBN="$2"
      shift 2
      ;;
    --port)
      if [[ $# -lt 2 ]]; then
        echo "Errore: --port richiede un valore"
        exit 1
      fi
      PORT="$2"
      shift 2
      ;;
    --help)
      print_help
      exit 0
      ;;
    *)
      echo "Argomento non riconosciuto: $1"
      print_help
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
python3 "$SCRIPT_DIR/mock_server_tests.py"

if $WITH_ARDUINO; then
  ARDUINO_OUTPUT_DIR="$PROJECT_ROOT/build-r4"
  if [[ "$BOARD_FQBN" == "arduino:avr:uno" ]]; then
    ARDUINO_OUTPUT_DIR="$PROJECT_ROOT/build-wokwi"
  fi

  arduino-cli compile -b "$BOARD_FQBN" --output-dir "$ARDUINO_OUTPUT_DIR" "$PROJECT_ROOT/Pinacoteca.ino"

  if $UPLOAD; then
    if [[ -z "$PORT" ]]; then
      echo "Errore: per --upload devi specificare anche --port <serial_port>"
      exit 1
    fi

    arduino-cli upload -p "$PORT" -b "$BOARD_FQBN" "$PROJECT_ROOT/Pinacoteca.ino"
  fi
fi

echo "Tutti i test completati con successo."
