#include <algorithm>
#include <vector>
#include <utility>
#include <benchmark/benchmark.h>
#include <sys/resource.h>
#include "radix_hash.h"
#include "strgen.h"
#include "tbb/parallel_sort.h"
#include "pdqsort/pdqsort.h"

struct identity_hash
{
  std::size_t operator()(const int& k) const {
    return k;
  }
};

bool tuple_cmp (std::tuple<std::size_t, int, int> a,
                std::tuple<std::size_t, int, int> b) {
  return std::get<0>(a) < std::get<0>(b);
}

static void BM_radix_hash_bf3_str(benchmark::State& state) {
  int size = state.range(0);
  std::vector<std::tuple<std::size_t, std::string, uint64_t>> dst(size);
  auto src = ::create_strvec(size);
  struct rusage u_before, u_after;
  getrusage(RUSAGE_SELF, &u_before);

  for (auto _ : state) {
    ::radix_hash_bf3<std::string,uint64_t>(src.begin(),
                                           src.end(), dst.begin(),
                                           ::compute_power(size),
                                           state.range(1), 0);
  }
  getrusage(RUSAGE_SELF, &u_after);
  state.SetComplexityN(state.range(0));
  state.counters["Minor"] = u_after.ru_minflt - u_before.ru_minflt;
  state.counters["Major"] = u_after.ru_majflt - u_before.ru_majflt;
  state.counters["Swap"] = u_after.ru_nswap - u_before.ru_nswap;
}

bool str_tuple_cmp (std::tuple<std::size_t, std::string, uint64_t> a,
                    std::tuple<std::size_t, std::string, uint64_t> b) {
  return std::get<0>(a) < std::get<0>(b);
}

static void BM_qsort_string(benchmark::State& state) {
  int size = state.range(0);
  std::vector<std::tuple<std::size_t, std::string, uint64_t>> dst(size);
  auto src = ::create_strvec(size);
  struct rusage u_before, u_after;
  getrusage(RUSAGE_SELF, &u_before);

  for (auto _ : state) {
    for (int i = 0; i < size; i++) {
      std::get<0>(dst[i]) = std::hash<std::string>{}(std::get<0>(src[i]));
      std::get<1>(dst[i]) = std::get<0>(src[i]);
      std::get<2>(dst[i]) = std::get<1>(src[i]);
    }
    std::sort(dst.begin(), dst.end(), str_tuple_cmp);
  }
  getrusage(RUSAGE_SELF, &u_after);
  state.SetComplexityN(state.range(0));
  state.counters["Minor"] = u_after.ru_minflt - u_before.ru_minflt;
  state.counters["Major"] = u_after.ru_majflt - u_before.ru_majflt;
  state.counters["Swap"] = u_after.ru_nswap - u_before.ru_nswap;
}

static void BM_radix_hash_bf4_str(benchmark::State& state) {
  int size = state.range(0);
  std::vector<std::tuple<std::size_t, std::string, uint64_t>> dst(size);
  auto src = ::create_strvec(size);
  unsigned int cores = std::thread::hardware_concurrency();
  struct rusage u_before, u_after;
  getrusage(RUSAGE_SELF, &u_before);

  for (auto _ : state) {
    ::radix_hash_bf4<std::string,uint64_t>(src.begin(),
                                           src.end(), dst.begin(),
                                           ::compute_power(size),
                                           state.range(1), 0, cores);
  }
  getrusage(RUSAGE_SELF, &u_after);
  state.SetComplexityN(state.range(0));
  state.counters["Minor"] = u_after.ru_minflt - u_before.ru_minflt;
  state.counters["Major"] = u_after.ru_majflt - u_before.ru_majflt;
  state.counters["Swap"] = u_after.ru_nswap - u_before.ru_nswap;
}

static void BM_radix_hash_bf5_str(benchmark::State& state) {
  int size = state.range(0);
  std::vector<std::tuple<std::size_t, std::string, uint64_t>> dst(size);
  auto src = ::create_strvec(size);
  unsigned int cores = std::thread::hardware_concurrency();
  struct rusage u_before, u_after;
  getrusage(RUSAGE_SELF, &u_before);

  for (auto _ : state) {
    ::radix_hash_bf5<std::string,uint64_t>(src.begin(),
                                           src.end(), dst.begin(),
                                           ::compute_power(size),
                                           state.range(1), 0, cores);
  }
  getrusage(RUSAGE_SELF, &u_after);
  state.SetComplexityN(state.range(0));
  state.counters["Minor"] = u_after.ru_minflt - u_before.ru_minflt;
  state.counters["Major"] = u_after.ru_majflt - u_before.ru_majflt;
  state.counters["Swap"] = u_after.ru_nswap - u_before.ru_nswap;
}

static void BM_radix_hash_bf6_str(benchmark::State& state) {
  int size = state.range(0);
  std::vector<std::tuple<std::size_t, std::string, uint64_t>> dst(size);
  auto src = ::create_strvec(size);
  unsigned int cores = std::thread::hardware_concurrency();
  struct rusage u_before, u_after;
  getrusage(RUSAGE_SELF, &u_before);

  for (auto _ : state) {
    ::radix_hash_bf6<std::string,uint64_t>(src.begin(),
                                           src.end(), dst.begin(),
                                           state.range(1), 0, cores);
  }
  getrusage(RUSAGE_SELF, &u_after);

  state.SetComplexityN(state.range(0));
  state.counters["Minor"] = u_after.ru_minflt - u_before.ru_minflt;
  state.counters["Major"] = u_after.ru_majflt - u_before.ru_majflt;
  state.counters["Swap"] = u_after.ru_nswap - u_before.ru_nswap;
}

static void BM_tbb_sort_string(benchmark::State& state) {
  int size = state.range(0);
  std::vector<std::tuple<std::size_t, std::string, uint64_t>> dst(size);
  auto src = ::create_strvec(size);
  struct rusage u_before, u_after;
  getrusage(RUSAGE_SELF, &u_before);

  for (auto _ : state) {
    for (int i = 0; i < size; i++) {
      std::get<0>(dst[i]) = std::hash<std::string>{}(std::get<0>(src[i]));
      std::get<1>(dst[i]) = std::get<0>(src[i]);
      std::get<2>(dst[i]) = std::get<1>(src[i]);
    }
    tbb::parallel_sort(dst.begin(), dst.end(), str_tuple_cmp);
  }
  getrusage(RUSAGE_SELF, &u_after);
  state.SetComplexityN(state.range(0));
  state.counters["Minor"] = u_after.ru_minflt - u_before.ru_minflt;
  state.counters["Major"] = u_after.ru_majflt - u_before.ru_majflt;
  state.counters["Swap"] = u_after.ru_nswap - u_before.ru_nswap;
}

static void BM_pdqsort_string(benchmark::State& state) {
  int size = state.range(0);
  std::vector<std::tuple<std::size_t, std::string, uint64_t>> dst(size);
  auto src = ::create_strvec(size);
  struct rusage u_before, u_after;
  getrusage(RUSAGE_SELF, &u_before);

  for (auto _ : state) {
    for (int i = 0; i < size; i++) {
      std::get<0>(dst[i]) = std::hash<std::string>{}(std::get<0>(src[i]));
      std::get<1>(dst[i]) = std::get<0>(src[i]);
      std::get<2>(dst[i]) = std::get<1>(src[i]);
    }
    pdqsort(dst.begin(), dst.end(), str_tuple_cmp);
  }
  getrusage(RUSAGE_SELF, &u_after);

  state.SetComplexityN(state.range(0));
  state.counters["Minor"] = u_after.ru_minflt - u_before.ru_minflt;
  state.counters["Major"] = u_after.ru_majflt - u_before.ru_majflt;
  state.counters["Swap"] = u_after.ru_nswap - u_before.ru_nswap;
}


static void RadixArguments(benchmark::internal::Benchmark* b) {
  for (int i = 11; i < 12; i+=1) {
    for (int j = (1 << 18); j <= (1 << 23); j <<= 1) {
      b->Args({j, i});
    }
  }
}

/*
 * Note: breadth first search outperforms depth first search when
 * input is small. For very large input, depth first wins a little bit.
 */

BENCHMARK(BM_qsort_string)->Apply(RadixArguments)
->Complexity(benchmark::oN)->UseRealTime();
BENCHMARK(BM_tbb_sort_string)->Apply(RadixArguments)
->Complexity(benchmark::oN)->UseRealTime();
BENCHMARK(BM_pdqsort_string)->Apply(RadixArguments)
->Complexity(benchmark::oN)->UseRealTime();
BENCHMARK(BM_radix_hash_bf3_str)->Apply(RadixArguments)
->Complexity(benchmark::oN)->UseRealTime();
BENCHMARK(BM_radix_hash_bf4_str)->Apply(RadixArguments)
->Complexity(benchmark::oN)->UseRealTime();
BENCHMARK(BM_radix_hash_bf5_str)->Apply(RadixArguments)
->Complexity(benchmark::oN)->UseRealTime();
BENCHMARK(BM_radix_hash_bf6_str)->Apply(RadixArguments)
->Complexity(benchmark::oN)->UseRealTime();

BENCHMARK_MAIN();
