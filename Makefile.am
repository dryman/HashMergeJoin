AUTOMAKE_OPTIONS = subdir-objects

ACLOCAL_AMFLAGS=-I m4
#SUBDIRS = googletest
TESTS = radix_hash_test strgen_test thread_barrier_test radix_sort_test partitioned_hash_test
check_PROGRAMS = radix_hash_test strgen_test thread_barrier_test radix_sort_test partitioned_hash_test

partitioned_hash_test_SOURCES = partitioned_hash_test.cc partitioned_hash.h thread_barrier.h thread_barrier.cc
partitioned_hash_test_CPPFLAGS = -isystem googletest/googletest/include
partitioned_hash_test_CXXFLAGS = -std=c++11 -Wall @PTHREAD_CFLAGS@ -Wextra
partitioned_hash_test_LDADD = googletest/googletest/lib/libgtest.la googletest/googletest/lib/libgtest_main.la @PTHREAD_LIBS@

radix_hash_test_SOURCES = radix_hash_test.cc radix_hash.h \
                          thread_barrier.h thread_barrier.cc
radix_hash_test_CPPFLAGS = -isystem googletest/googletest/include
radix_hash_test_CXXFLAGS = -std=c++11 -Wall @PTHREAD_CFLAGS@ -Wextra
radix_hash_test_LDADD = googletest/googletest/lib/libgtest.la \
googletest/googletest/lib/libgtest_main.la \
@PTHREAD_LIBS@
radix_hash_test_LDFLAGS = -static

radix_sort_test_SOURCES = radix_sort_test.cc radix_sort.h \
                          thread_barrier.h thread_barrier.cc
radix_sort_test_CPPFLAGS = -isystem googletest/googletest/include
radix_sort_test_CXXFLAGS = -std=c++11 -Wall @PTHREAD_CFLAGS@ -fno-strict-aliasing
radix_sort_test_LDADD = googletest/googletest/lib/libgtest.la \
googletest/googletest/lib/libgtest_main.la \
@PTHREAD_LIBS@
radix_sort_test_LDFLAGS = -static

strgen_test_SOURCES = strgen.cc strgen_test.cc
strgen_test_CPPFLAGS = -isystem googletest/googletest/include
strgen_test_CXXFLAGS = -std=c++11 @PTHREAD_CFLAGS@
strgen_test_LDADD = googletest/googletest/lib/libgtest.la \
googletest/googletest/lib/libgtest_main.la \
@PTHREAD_LIBS@
strgen_test_LDFLAGS = -static

thread_barrier_test_SOURCES = thread_barrier.cc thread_barrier.h \
                              thread_barrier_test.cc
thread_barrier_test_CPPFLAGS = -isystem googletest/googletest/include
thread_barrier_test_CXXFLAGS = -std=c++11 @PTHREAD_CFLAGS@
thread_barrier_test_LDADD = googletest/googletest/lib/libgtest.la \
googletest/googletest/lib/libgtest_main.la \
@PTHREAD_LIBS@
thread_barrier_test_LDFLAGS = -static

bin_PROGRAMS = find_k_bench radix_hash_bench hashjoin_bench radix_sort_bench \
radix_bench_seq radix_bench_par

find_k_bench_SOURCES = find_k_bench.cc strgen.cc radix_hash.h radix_sort.h thread_barrier.h thread_barrier.cc
find_k_bench_CXXFLAGS = -std=c++11 -Wall @PTHREAD_CFLAGS@ @PAPI_CFLAGS@
find_k_bench_LDADD = @PTHREAD_LIBS@ @PAPI_LIBS@
find_k_bench_LDFLAGS = -lbenchmark

radix_hash_bench_SOURCES = radix_hash_bench.cc strgen.cc radix_hash.h thread_barrier.h thread_barrier.cc
radix_hash_bench_CXXFLAGS = -std=c++11 -Wall @PTHREAD_CFLAGS@ @PAPI_CFLAGS@
radix_hash_bench_LDADD = @PTHREAD_LIBS@ @PAPI_LIBS@
radix_hash_bench_LDFLAGS = -lbenchmark -ltbb -ltbbmalloc

radix_sort_bench_SOURCES = radix_sort_bench.cc radix_sort.h thread_barrier.h thread_barrier.cc
radix_sort_bench_CXXFLAGS = -std=c++11 -Wall @PTHREAD_CFLAGS@ @PAPI_CFLAGS@
radix_sort_bench_LDADD = @PTHREAD_LIBS@ @PAPI_LIBS@
radix_sort_bench_LDFLAGS = -lbenchmark -ltbb -ltbbmalloc

hashjoin_bench_SOURCES = hashjoin_bench.cc strgen.cc thread_barrier.h thread_barrier.cc partitioned_hash.h
hashjoin_bench_CXXFLAGS = -std=c++11 @PTHREAD_CFLAGS@ @PAPI_CFLAGS@
hashjoin_bench_LDADD = @PTHREAD_LIBS@ @PAPI_LIBS@
hashjoin_bench_LDFLAGS = -lbenchmark

radix_bench_seq_SOURCES = radix_bench_seq.cc strgen.cc radix_hash.h radix_sort.h thread_barrier.h thread_barrier.cc
radix_bench_seq_CXXFLAGS = -std=c++11 -Wall @PTHREAD_CFLAGS@ @PAPI_CFLAGS@
radix_bench_seq_LDADD = @PTHREAD_LIBS@ @PAPI_LIBS@
radix_bench_seq_LDFLAGS = -lbenchmark

radix_bench_par_SOURCES = radix_bench_par.cc strgen.cc radix_hash.h radix_sort.h thread_barrier.h thread_barrier.cc
radix_bench_par_CXXFLAGS = -std=c++11 -Wall @PTHREAD_CFLAGS@ @PAPI_CFLAGS@
radix_bench_par_LDADD = @PTHREAD_LIBS@ @PAPI_LIBS@
radix_bench_par_LDFLAGS = -lbenchmark -ltbb -ltbbmalloc
