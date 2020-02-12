.PHONY: test test-cpp clean valgrind run

BINS=main db
CBINS=$(BINS:%=bin/%)
CXXBINS=$(BINS:%=bin/cpp/%)
CCOMMANDS=$(CBINS:%=% &)
CXXCOMMANDS=$(CXXBINS:%=% &)
TESTS=$(BINS:%=test/%)
TESTCMDS=$(TESTS:%=% >> test-results.txt; )

CFLAGS :=-O3 -std=c99 -lpq
CXXFLAGS :=-O3 -std=c++98 -lpq

all: $(CBINS)

run: bin/main
	bin/main

cpp: $(CXXBINS)

test: $(CBINS) $(TESTS)
	> test-results.txt
	$(CCOMMANDS) $(TESTCMDS) killall $(BINS)
	diff test-results.txt test/results.txt

test-cpp: $(CXXBINS) $(TESTS)
	> test-results.txt
	$(CXXCOMMANDS) $(TESTCMDS) killall $(BINS)
	diff test-results.txt test/results.txt

valgrind: $(CBINS) test/valgrind
	test/valgrind
	diff valgrind-results.txt test/valgrind.txt

bin/%: test/%.c httpserver.h httpserver-pg.h | bin
	$(CC) $(CFLAGS) -Wall -Wextra -Werror $< -o $@

bin/cpp/%: test/%.cpp httpserver.h httpserver-pg.h | bin/cpp
	$(CXX) $(CXXFLAGS) -Wall -Wextra -Werror $< -o $@

test/%.cpp: test/%.c
	cp $< $@

bin/cpp: bin
	mkdir $@

bin:
	mkdir $@

clean:
	@rm -rf bin *.txt
