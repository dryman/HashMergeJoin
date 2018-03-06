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

bool tuple_cmp (std::tuple<std::size_t, int, int> a,
                std::tuple<std::size_t, int, int> b) {
  return std::get<0>(a) < std::get<0>(b);
}

static void BM_qsort(benchmark::State& state) {
  int size = state.range(0);
  std::vector<std::pair<int, int>> input(size);
  std::vector<std::tuple<std::size_t, int, int>> dst(size);

  for (int i = size; i > 0; i--) {
    input[size - i].first = i;
    input[size - i].second = i;
  }

  for (auto _ : state)
    {
      for (int i = 0; i < size; i++) {
        std::get<0>(dst[i]) = input[i].first;
        std::get<1>(dst[i]) = input[i].first;
        std::get<2>(dst[i]) = input[i].first;
      }

      std::sort(dst.begin(), dst.end(), tuple_cmp);
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

static void BM_radix_hash_bf3(benchmark::State& state) {
  int size = state.range(0);
  std::vector<std::pair<int, int>> src;
  std::vector<std::tuple<std::size_t, int, int>> dst(size);

  for (int i = size; i > 0; i--) {
    src.push_back(std::make_pair(i, i));
  }

  for (auto _ : state)
    {
      ::radix_hash_bf3<int,int,identity_hash>(src.begin(),
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

static void BM_radix_hash_bf3_str(benchmark::State& state) {
  int size = state.range(0);
  std::vector<std::tuple<std::size_t, std::string, uint64_t>> dst(size);
  auto src = ::create_strvec(size);

  for (auto _ : state) {
    ::radix_hash_bf3<std::string,uint64_t>(src.begin(),
                                           src.end(), dst.begin(),
                                           ::compute_power(size),
                                           state.range(1), 0);
  }
  state.SetComplexityN(state.range(0));
}

bool str_tuple_cmp (std::tuple<std::size_t, std::string, uint64_t> a,
                    std::tuple<std::size_t, std::string, uint64_t> b) {
  return std::get<0>(a) < std::get<0>(b);
}

static void BM_qsort_string(benchmark::State& state) {
  int size = state.range(0);
  std::vector<std::tuple<std::size_t, std::string, uint64_t>> dst(size);
  auto src = ::create_strvec(size);

  for (auto _ : state) {
    for (int i = 0; i < size; i++) {
      std::get<0>(dst[i]) = std::hash<std::string>{}(std::get<0>(src[i]));
      std::get<1>(dst[i]) = std::get<0>(src[i]);
      std::get<2>(dst[i]) = std::get<1>(src[i]);
    }
    std::sort(dst.begin(), dst.end(), str_tuple_cmp);
  }
  state.SetComplexityN(state.range(0));
}

static void BM_radix_hash_bf4_str(benchmark::State& state) {
  int size = state.range(0);
  std::vector<std::tuple<std::size_t, std::string, uint64_t>> dst(size);
  auto src = ::create_strvec(size);

  for (auto _ : state) {
    ::radix_hash_bf4<std::string,uint64_t>(src.begin(),
                                           src.end(), dst.begin(),
                                           ::compute_power(size),
                                           state.range(1), 0, 4);
  }
  state.SetComplexityN(state.range(0));
}


static void RadixArguments(benchmark::internal::Benchmark* b) {
  for (int i = 11; i < 12; i+=1) {
    for (int j = 1024; j <= (1UL << 20); j <<= 1) {
      b->Args({j, i});
    }
  }
}

/*
 * Note: breadth first search outperforms depth first search when
 * input is small. For very large input, depth first wins a little bit.
 */

//BENCHMARK(BM_qsort)->Range(1 << 10, 1 << 20)->Complexity();
//BENCHMARK(BM_radix_hash_df1)->Apply(RadixArguments)->Complexity();
//BENCHMARK(BM_radix_hash_df2)->Apply(RadixArguments)->Complexity();
//BENCHMARK(BM_radix_hash_bf1)->Apply(RadixArguments)->Complexity();
// BENCHMARK(BM_radix_hash_bf2)->Apply(RadixArguments)->Complexity();
//BENCHMARK(BM_radix_hash_bf3)->Apply(RadixArguments)->Complexity();
//
BENCHMARK(BM_radix_hash_df1_str)->Apply(RadixArguments)
->Complexity(benchmark::oN);
//BENCHMARK(BM_radix_hash_df2_str)->Apply(RadixArguments)->Complexity();
//BENCHMARK(BM_radix_hash_bf1_str)->Apply(RadixArguments)->Complexity();
//BENCHMARK(BM_radix_hash_bf2_str)->Apply(RadixArguments)->Complexity();
BENCHMARK(BM_radix_hash_bf3_str)->Apply(RadixArguments)
->Complexity(benchmark::oN);
BENCHMARK(BM_qsort_string)->Apply(RadixArguments)
->Complexity(benchmark::oN);
BENCHMARK(BM_radix_hash_bf4_str)->Apply(RadixArguments)
->Complexity(benchmark::oN);

BENCHMARK_MAIN();
