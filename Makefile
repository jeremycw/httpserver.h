.PHONY: test clean format check-format debug fuzz

test: test-unit test-functional test-functional-cpp

test-unit: debug
	./build/test/unit/unit-test-runner

test-functional: debug
	./test/functional/functional-test-runner

test-functional-cpp: debug
	./test/functional/functional-test-runner -cpp

debug: build
	cd build; \
	cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_INCLUDE_WHAT_YOU_USE="include-what-you-use;-Xiwyu;--mapping_file=../iwyu.imp" ..; \
	make; \
	cd ..;

build:
	mkdir build

format:
	find src -name "*.[h|c]" -exec sh -c 'clang-format --style=LLVM $$0 > $$0.frmt; mv $$0.frmt $$0' {} \;

check-format:
	clang-format --style=LLVM --dry-run -Werror src/*.c

fuzz: build
	cd build && clang -g -fsanitize=address -DKQUEUE -I ../src -I ../build/src -o libfuzzer_test ../test/fuzz/fuzz_harness.c src/libhttpsrv.a
	clang -g -O0 -DKQUEUE -I src -I build/src -o random_fuzz_test test/fuzz/random_parser.c build/src/libhttpsrv.a
	./random_fuzz_test $(SEED)

clean:
	@rm -rf build random_fuzz_test libfuzzer_test
