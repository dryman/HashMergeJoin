/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 * 
 *   http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include<benchmark/benchmark.h>

static void BM_UniquePtr(benchmark::State& state) {
  using namespace std;
  for (auto _ : state)
    {
      auto arr = make_unique<pair<string, uint64_t>[]>(state.range(0));
      arr[0] = {"abc", 123};
    }
}

static void BM_Vector(benchmark::State& state) {
  using namespace std;
  for (auto _ : state)
    {
      std::vector<pair<string, uint64_t>> arr (state.range(0));
      arr[0] = {"abc", 123};
    }
}

static void BM_MallocOnly(benchmark::State& state) {
  for (auto _ : state)
    {
      char* x;
      benchmark::DoNotOptimize(x = (char*)malloc(state.range(0)));
      benchmark::DoNotOptimize(x[0] = 'a');
      free(x);
    }
}

static void BM_tmpbuff(benchmark::State& state) {
  for (auto _ : state)
    {
      /*
      auto p = std::get_temporary_buffer<std::pair<std::string,uint64_t>>(state.range(0));
      auto arr = p.first;
      //arr[0] = {"01234567890123456789012345", 123}; // this is copy
      arr[0] = {"01234", 123}; // this is copy
      arr[0].first.~basic_string<char>();
      //auto my_pair = std::move(arr[0]); // this is move, now arr[0] is unspecified
      std::return_temporary_buffer(p.first);
      */
      auto p = std::get_temporary_buffer<char[20]>(state.range(0));
      auto arr = p.first;
      arr[0][0] = 'a';
      arr[0][1] = 'b';
      arr[0][2] = 'c';
      std::return_temporary_buffer(p.first);
    }
}

static void BM_malloc(benchmark::State& state) {
  for (auto _ : state)
    {
      auto arr = (std::pair<std::string,uint64_t>*)malloc(
          sizeof(std::pair<std::string,uint64_t>)*state.range(0));
      arr[0] = {"01234567890123456789012345", 123};
      free(arr);
    }
}

// Modern C++ RAII pointer
// Problem: array initialization takes time
BENCHMARK(BM_UniquePtr)->Range(1024, 1 << 18);

BENCHMARK(BM_Vector)->Range(1024, 1<<18);

// malloc overhead baseline.
BENCHMARK(BM_MallocOnly)->Arg(8)->Arg(64)->Arg(4096);

// tmpbuff can avoid initialization, but can only be used with
// POD object. Using string here valgrind would report memleak
BENCHMARK(BM_tmpbuff)->Arg(8)->Arg(64)->Arg(4096);

// manual malloc with string can cause a leak, too.
BENCHMARK(BM_malloc)->Arg(8)->Arg(64)->Arg(4096);

BENCHMARK_MAIN();
