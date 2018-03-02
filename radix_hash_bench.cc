#include <algorithm>
#include <vector>
#include <utility>
#include <benchmark/benchmark.h>
#include "radix_hash.h"
#include "strgen.h"

struct identity_hash
{
  std::size_t operator()(const int& k) const {
    return k;
  }
};

bool pair_cmp (std::pair<int, int> a, std::pair<int, int> b) {
  return a.first < b.first;
}

static void BM_qsort(benchmark::State& state) {
  std::vector<std::pair<int, int>> data;
  int size = state.range(0);
  for (int i = size; i > 0; i--) {
    data.push_back(std::make_pair(i, i));
  }

  for (auto _ : state)
    {
      state.PauseTiming();
      for (int i = size; i > 0; i--) {
        data[size - i].first = i;
        data[size - i].second = i;
      }
      state.ResumeTiming();
      std::sort(data.begin(), data.end(), pair_cmp);
    }
  state.SetComplexityN(state.range(0));
}

static void BM_radix_hash_df1(benchmark::State& state) {
  int size = state.range(0);
  std::vector<std::pair<int, int>> src;
  std::vector<std::tuple<std::size_t, int, int>> dst(size);

  for (int i = size; i > 0; i--) {
    src.push_back(std::make_pair(i, i));
  }

  for (auto _ : state)
    {
      ::radix_hash_df1<int,int,identity_hash>(src.begin(),
                                              src.end(), dst.begin(),
                                              ::compute_power(size),
                                              state.range(1), 0);
    }
  state.SetComplexityN(state.range(0));
}

static void BM_radix_hash_df2(benchmark::State& state) {
  int size = state.range(0);
  std::vector<std::pair<int, int>> src;
  std::vector<std::tuple<std::size_t, int, int>> dst(size);

  for (int i = size; i > 0; i--) {
    src.push_back(std::make_pair(i, i));
  }

  for (auto _ : state)
    {
      ::radix_hash_df2<int,int,identity_hash>(src.begin(),
                                              src.end(), dst.begin(),
                                              ::compute_power(size),
                                              state.range(1), 0);
    }
  state.SetComplexityN(state.range(0));
}

static void BM_radix_hash_bf1(benchmark::State& state) {
  int size = state.range(0);
  std::vector<std::pair<int, int>> src;
  std::vector<std::tuple<std::size_t, int, int>> dst(size);

  for (int i = size; i > 0; i--) {
    src.push_back(std::make_pair(i, i));
  }

  for (auto _ : state)
    {
      ::radix_hash_bf1<int,int,identity_hash>(src.begin(),
                                              src.end(), dst.begin(),
                                              ::compute_power(size),
                                              state.range(1), 0);
    }
  state.SetComplexityN(state.range(0));
}

static void BM_radix_hash_bf2(benchmark::State& state) {
  int size = state.range(0);
  std::vector<std::pair<int, int>> src;
  std::vector<std::tuple<std::size_t, int, int>> dst(size);

  for (int i = size; i > 0; i--) {
    src.push_back(std::make_pair(i, i));
  }

  for (auto _ : state)
    {
      ::radix_hash_bf2<int,int,identity_hash>(src.begin(),
                                              src.end(), dst.begin(),
                                              ::compute_power(size),
                                              state.range(1), 0);
    }
  state.SetComplexityN(state.range(0));
}

static void BM_radix_hash_df1_str(benchmark::State& state) {
  int size = state.range(0);
  std::vector<std::tuple<std::size_t, std::string, uint64_t>> dst(size);
  auto src = ::create_strvec(size);

  for (auto _ : state) {
    ::radix_hash_df1<std::string,uint64_t>(src.begin(),
                                           src.end(), dst.begin(),
                                           ::compute_power(size),
                                           state.range(1), 0);
  }
  state.SetComplexityN(state.range(0));
}

static void BM_radix_hash_df2_str(benchmark::State& state) {
  int size = state.range(0);
  std::vector<std::tuple<std::size_t, std::string, uint64_t>> dst(size);
  auto src = ::create_strvec(size);

  for (auto _ : state) {
    ::radix_hash_df2<std::string,uint64_t>(src.begin(),
                                           src.end(), dst.begin(),
                                           ::compute_power(size),
                                           state.range(1), 0);
  }
  state.SetComplexityN(state.range(0));
}

static void BM_radix_hash_bf1_str(benchmark::State& state) {
  int size = state.range(0);
  std::vector<std::tuple<std::size_t, std::string, uint64_t>> dst(size);
  auto src = ::create_strvec(size);

  for (auto _ : state) {
    ::radix_hash_bf1<std::string,uint64_t>(src.begin(),
                                           src.end(), dst.begin(),
                                           ::compute_power(size),
                                           state.range(1), 0);
  }
  state.SetComplexityN(state.range(0));
}

static void BM_radix_hash_bf2_str(benchmark::State& state) {
  int size = state.range(0);
  std::vector<std::tuple<std::size_t, std::string, uint64_t>> dst(size);
  auto src = ::create_strvec(size);

  for (auto _ : state) {
    ::radix_hash_bf2<std::string,uint64_t>(src.begin(),
                                           src.end(), dst.begin(),
                                           ::compute_power(size),
                                           state.range(1), 0);
  }
  state.SetComplexityN(state.range(0));
}

static void RadixArguments(benchmark::internal::Benchmark* b) {
  for (int i = 10; i < 13; i+=1) {
    for (int j = 1024; j < (1UL << 18); j <<= 2) {
      b->Args({j, i});
    }
  }
}

/*
 * Note: breadth first search outperforms depth first search when
 * input is small. For very large input, depth first wins a little bit.
 */

//BENCHMARK(BM_qsort)->Range(1 << 10, 1 << 18)->Complexity();
//BENCHMARK(BM_radix_hash_df1)->Apply(RadixArguments)->Complexity();
//BENCHMARK(BM_radix_hash_df2)->Apply(RadixArguments)->Complexity();
//BENCHMARK(BM_radix_hash_bf1)->Apply(RadixArguments)->Complexity();
// BENCHMARK(BM_radix_hash_bf2)->Apply(RadixArguments)->Complexity();
//
//BENCHMARK(BM_radix_hash_df1_str)->Apply(RadixArguments)->Complexity();
//BENCHMARK(BM_radix_hash_df2_str)->Apply(RadixArguments)->Complexity();
//BENCHMARK(BM_radix_hash_bf1_str)->Apply(RadixArguments)->Complexity();
BENCHMARK(BM_radix_hash_bf2_str)->Apply(RadixArguments)->Complexity();

BENCHMARK_MAIN();
