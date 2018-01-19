ACLOCAL_AMFLAGS=-I m4
#SUBDIRS = googletest
TESTS = radix_hash_test
check_PROGRAMS = radix_hash_test

radix_hash_test_SOURCES = radix_hash_test.cc
radix_hash_test_CPPFLAGS = -isystem googletest/googletest/include
radix_hash_test_CXXFLAGS = -std=c++14
radix_hash_test_LDADD = googletest/googletest/lib/libgtest.la googletest/googletest/lib/libgtest_main.la
#radix_hash_test_LDFLAGS = -lgtest

bin_PROGRAMS = buffer_latency strgen
buffer_latency_SOURCES = buffer_latency.cc
buffer_latency_CXXFLAGS = -std=c++14
buffer_latency_LDFLAGS = -lbenchmark

strgen_SOURCES = strgen.cc
strgen_CXXFLAGS = -std=c++14
strgen_LDFLAGS = -lbenchmark
