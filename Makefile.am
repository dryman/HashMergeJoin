ACLOCAL_AMFLAGS=-I m4
#SUBDIRS = googletest
TESTS = radix_hash_test leak_check strgen_test
check_PROGRAMS = radix_hash_test leak_check strgen_test

radix_hash_test_SOURCES = radix_hash_test.cc
radix_hash_test_CPPFLAGS = -isystem googletest/googletest/include
radix_hash_test_CXXFLAGS = -std=c++14 -Wall
radix_hash_test_LDADD = googletest/googletest/lib/libgtest.la \
googletest/googletest/lib/libgtest_main.la
radix_hash_test_LDFLAGS = -static

leak_check_SOURCES = leak_check.cc
leak_check_CPPFLAGS = -isystem googletest/googletest/include
leak_check_CXXFLAGS = -std=c++14
leak_check_LDADD = googletest/googletest/lib/libgtest.la \
googletest/googletest/lib/libgtest_main.la
leak_check_LDFLAGS = -static

strgen_test_SOURCES = strgen.cc strgen_test.cc
strgen_test_CPPFLAGS = -isystem googletest/googletest/include
strgen_test_CXXFLAGS = -std=c++14
strgen_test_LDADD = googletest/googletest/lib/libgtest.la \
googletest/googletest/lib/libgtest_main.la
strgen_test_LDFLAGS = -static

bin_PROGRAMS = buffer_latency radix_hash_bench hashjoin_bench callback_bench

buffer_latency_SOURCES = buffer_latency.cc
buffer_latency_CXXFLAGS = -std=c++14
buffer_latency_LDFLAGS = -lbenchmark

radix_hash_bench_SOURCES = radix_hash_bench.cc strgen.cc radix_hash.h
radix_hash_bench_CXXFLAGS = -std=c++14 -Wall
radix_hash_bench_LDFLAGS = -lbenchmark

hashjoin_bench_SOURCES = hashjoin_bench.cc hashjoin.cc strgen.cc
hashjoin_bench_CXXFLAGS = -std=c++14
hashjoin_bench_LDFLAGS = -lbenchmark

callback_bench_SOURCES = callback_bench.cc strgen.cc
callback_bench_CXXFLAGS = -std=c++14
callback_bench_LDFLAGS = -lbenchmark

analyze_hashjoin_srcs = $(hashjoin_bench_SOURCES)
analyze_hashjoin_plists = $(analyze_hashjoin_srcs:%.cc=%.plist)
MOSTLYCLEANFILES = $(analyze_hashjoin_plists)

$(analyze_hashjoin_plists):	%.plist: %.cc
	@echo "  CCSA  " $@
	$(CXX) --analyze -std=c++14 $< -o $@

all-local:	$(analyze_hashjoin_plists)

analyze:	$(analyze_hashjoin_plists)

.PHONY:	analyze
