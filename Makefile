.PHONY: test test-clean test-full test-full-r4 test-full-all full-test compile-r4 compile-wokwi mock-server

BOARD ?= arduino:renesas_uno:unor4wifi
R4_BOARD := arduino:renesas_uno:unor4wifi
WOKWI_BOARD := arduino:avr:uno
R4_OUTPUT := ./build-r4
WOKWI_OUTPUT := ./build-wokwi

test:
	./test/run_tests.sh

test-clean:
	./test/run_tests.sh --clean

test-full:
	./test/run_tests.sh --clean --with-arduino --board $(BOARD)

test-full-r4:
	./test/run_tests.sh --clean --with-arduino --board $(R4_BOARD)

compile-r4:
	arduino-cli compile -b $(R4_BOARD) --output-dir $(R4_OUTPUT) ./Pinacoteca.ino

compile-wokwi:
	arduino-cli compile -b $(WOKWI_BOARD) --output-dir $(WOKWI_OUTPUT) ./Pinacoteca.ino

mock-server:
	python3 tools/mock_arduino_server.py

test-full-all:
	./test/run_tests.sh --clean
	$(MAKE) compile-wokwi
	$(MAKE) compile-r4

full-test: test-full-all
