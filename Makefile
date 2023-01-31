.PHONY: test clean format check-format debug

test: test-unit test-functional

test-unit: debug
	./build/test/unit/unit-test-runner

test-functional:
	./test/functional/functional-test-runner

debug: build
	cd build; \
	cmake -DCMAKE_BUILD_TYPE=Debug ..; \
	make; \
	cd ..;

build:
	mkdir build

format:
	find src -name "*.[h|c]" -exec sh -c 'clang-format --style=LLVM $$0 > $$0.frmt; mv $$0.frmt $$0' {} \;

check-format:
	clang-format --style=LLVM --dry-run -Werror src/*.c

clean:
	@rm -rf build
