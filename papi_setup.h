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

#ifndef PAPI_SETUP_H
#define PAPI_SETUP_H 1

#include "config.h"
#ifdef HAVE_PAPI
#include "papi.h"

// Yes, we are declaring evil global variable in headers.
// This header should only be included once in each benchmark.
const int e_num = 4;
int events[e_num] = {
  PAPI_L1_DCM,
  PAPI_L2_DCM,
  PAPI_L3_TCM,
  PAPI_TOT_INS,
};
long long papi_values[e_num];
long long acc_values[e_num];
char papi_error_str[PAPI_MAX_STR_LEN];

#define RESET_ACC_COUNTERS do { \
  for (int i = 0; i < e_num; i++) \
    acc_values[i] = 0; \
  } while (0)

#define START_COUNTERS do { \
    if(PAPI_start_counters(events, e_num) != PAPI_OK) { \
      PAPI_perror(papi_error_str); \
      exit(1); \
    } \
  } while (0)

#define ACCUMULATE_COUNTERS do { \
  assert(PAPI_stop_counters(papi_values, e_num) == PAPI_OK); \
  for (int i = 0; i < e_num; i++) \
    acc_values[i] += papi_values[i]; \
  } while (0)

#define REPORT_COUNTERS(STATE) do { \
  STATE.counters["L1 miss"] = acc_values[0] / STATE.iterations(); \
  STATE.counters["L2 miss"] = acc_values[1] / STATE.iterations(); \
  STATE.counters["L3 miss"] = acc_values[2] / STATE.iterations(); \
  STATE.counters["instructions"] = acc_values[3] / STATE.iterations(); \
  } while (0)

#else
#define RESET_ACC_COUNTERS (void)0
#define START_COUNTERS (void)0
#define ACCUMULATE_COUNTERS (void)0
#define REPORT_COUNTERS(STATE) (void)0
#endif

#endif
