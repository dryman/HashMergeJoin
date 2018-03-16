AUTOMAKE_OPTIONS = subdir-objects

ACLOCAL_AMFLAGS=-I m4
#SUBDIRS = googletest
TESTS = radix_hash_test leak_check strgen_test thread_barrier_test radix_sort_test
check_PROGRAMS = radix_hash_test leak_check strgen_test thread_barrier_test radix_sort_test

radix_hash_test_SOURCES = radix_hash_test.cc radix_hash.h \
                          thread_barrier.h thread_barrier.cc
radix_hash_test_CPPFLAGS = -isystem googletest/googletest/include
radix_hash_test_CXXFLAGS = -std=c++14 -Wall @PTHREAD_CFLAGS@ -Wextra
radix_hash_test_LDADD = googletest/googletest/lib/libgtest.la \
googletest/googletest/lib/libgtest_main.la \
@PTHREAD_LIBS@
radix_hash_test_LDFLAGS = -static

radix_sort_test_SOURCES = radix_sort_test.cc radix_sort.h \
                          thread_barrier.h thread_barrier.cc
radix_sort_test_CPPFLAGS = -isystem googletest/googletest/include
radix_sort_test_CXXFLAGS = -std=c++14 -Wall @PTHREAD_CFLAGS@ -fno-strict-aliasing
radix_sort_test_LDADD = googletest/googletest/lib/libgtest.la \
googletest/googletest/lib/libgtest_main.la \
@PTHREAD_LIBS@
radix_sort_test_LDFLAGS = -static

leak_check_SOURCES = leak_check.cc
leak_check_CPPFLAGS = -isystem googletest/googletest/include
leak_check_CXXFLAGS = -std=c++14 @PTHREAD_CFLAGS@
leak_check_LDADD = googletest/googletest/lib/libgtest.la \
googletest/googletest/lib/libgtest_main.la \
@PTHREAD_LIBS@
leak_check_LDFLAGS = -static

strgen_test_SOURCES = strgen.cc strgen_test.cc
strgen_test_CPPFLAGS = -isystem googletest/googletest/include
strgen_test_CXXFLAGS = -std=c++14 @PTHREAD_CFLAGS@
strgen_test_LDADD = googletest/googletest/lib/libgtest.la \
googletest/googletest/lib/libgtest_main.la \
@PTHREAD_LIBS@
strgen_test_LDFLAGS = -static

thread_barrier_test_SOURCES = thread_barrier.cc thread_barrier.h \
                              thread_barrier_test.cc
thread_barrier_test_CPPFLAGS = -isystem googletest/googletest/include
thread_barrier_test_CXXFLAGS = -std=c++14 @PTHREAD_CFLAGS@
thread_barrier_test_LDADD = googletest/googletest/lib/libgtest.la \
googletest/googletest/lib/libgtest_main.la \
@PTHREAD_LIBS@
thread_barrier_test_LDFLAGS = -static

bin_PROGRAMS = buffer_latency radix_hash_bench hashjoin_bench callback_bench radix_sort_bench

buffer_latency_SOURCES = buffer_latency.cc
buffer_latency_CXXFLAGS = -std=c++14 @PTHREAD_CFLAGS@
buffer_latency_LDADD = @PTHREAD_LIBS@
buffer_latency_LDFLAGS = -lbenchmark

radix_hash_bench_SOURCES = radix_hash_bench.cc strgen.cc radix_hash.h \
                           thread_barrier.h thread_barrier.cc
radix_hash_bench_CXXFLAGS = -std=c++14 -Wall @PTHREAD_CFLAGS@ @PAPI_CFLAGS@
radix_hash_bench_LDADD = @PTHREAD_LIBS@ @PAPI_LIBS@
radix_hash_bench_LDFLAGS = -lbenchmark -ltbb -ltbbmalloc

radix_sort_bench_SOURCES = radix_sort_bench.cc radix_sort.h \
                           thread_barrier.h thread_barrier.cc
radix_sort_bench_CXXFLAGS = -std=c++14 -Wall @PTHREAD_CFLAGS@ @PAPI_CFLAGS@
radix_sort_bench_LDADD = @PTHREAD_LIBS@ @PAPI_LIBS@
radix_sort_bench_LDFLAGS = -lbenchmark -ltbb -ltbbmalloc

hashjoin_bench_SOURCES = hashjoin_bench.cc strgen.cc \
                         thread_barrier.h thread_barrier.cc
hashjoin_bench_CXXFLAGS = -std=c++14 @PTHREAD_CFLAGS@ @PAPI_CFLAGS@
hashjoin_bench_LDADD = @PTHREAD_LIBS@ @PAPI_LIBS@
hashjoin_bench_LDFLAGS = -lbenchmark

callback_bench_SOURCES = callback_bench.cc strgen.cc
callback_bench_CXXFLAGS = -std=c++14 @PTHREAD_CFLAGS@
callback_bench_LDADD = @PTHREAD_LIBS@
callback_bench_LDFLAGS = -lbenchmark

# analyze_hashjoin_srcs = $(hashjoin_bench_SOURCES)
# analyze_hashjoin_plists = $(analyze_hashjoin_srcs:%.cc=%.plist)
# MOSTLYCLEANFILES = $(analyze_hashjoin_plists)
# 
# $(analyze_hashjoin_plists):	%.plist: %.cc
# 	@echo "  CCSA  " $@
# 	$(CXX) --analyze -std=c++14 $< -o $@
# 
# all-local:	$(analyze_hashjoin_plists)
# 
# analyze:	$(analyze_hashjoin_plists)
# 
# .PHONY:	analyze
