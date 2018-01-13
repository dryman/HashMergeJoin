#include<benchmark/benchmark.h>

static void BM_Example(benchmark::State& state) {
  for (auto _ : state)
    std::string empty_string;
}

BENCHMARK(BM_Example);

BENCHMARK_MAIN();
