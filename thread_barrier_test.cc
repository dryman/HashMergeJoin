/*
 * Copyright 2018 Felix Chern
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <atomic>
#include <thread>
#include <vector>
#include "gtest/gtest.h"
#include "thread_barrier.h"

TEST(thread_barrier_test, leader_count) {
  int num = 20;
  std::atomic_uint non_leader_cnt(0);
  std::atomic_uint leader_cnt(0);
  std::vector<std::thread> threads(num);
  ThreadBarrier tb(num);

  auto lambda = [&](std::atomic_uint* non_leader_cnt,
                    std::atomic_uint* leader_cnt) {
    if (tb.wait()) {
      (*leader_cnt)++;
    } else {
      (*non_leader_cnt)++;
    }
  };

  for (int i = 0; i < num-1; i++) {
    threads[i] = std::thread(lambda, &non_leader_cnt, &leader_cnt);
  }

  // All threads are blocked
  EXPECT_EQ(0, non_leader_cnt);
  EXPECT_EQ(0, leader_cnt);

  threads[num-1] = std::thread(lambda, &non_leader_cnt, &leader_cnt);

  for (auto&& t : threads)
    t.join();

  // Now we get all the counts
  EXPECT_EQ(num-1, non_leader_cnt);
  EXPECT_EQ(1, leader_cnt);
}
