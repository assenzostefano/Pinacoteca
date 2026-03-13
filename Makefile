.PHONY: test test-clean test-full

test:
	./test/run_tests.sh

test-clean:
	./test/run_tests.sh --clean

test-full:
	./test/run_tests.sh --clean --with-arduino
