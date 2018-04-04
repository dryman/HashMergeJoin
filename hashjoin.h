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

#ifndef HASH_JOIN_H
#define HASH_JOIN_H 1

#include <algorithm>
#include <vector>
#include <string>
#include <utility>
#include <type_traits>
#include <functional>
#include <thread>
#include "radix_hash.h"

typedef std::vector<std::pair<std::string, uint64_t>> KeyValVec;
typedef std::vector<std::tuple<std::size_t, std::string, uint64_t>>
  HashKeyValVec;

template<typename RIter, typename SIter>
class HashMergeJoin {
  static_assert(std::is_same<
                typename RIter::value_type::first_type,
                typename SIter::value_type::first_type>::value,
                "RIter and SIter key type must be the same");
  static_assert(std::is_same<
                typename RIter::difference_type,
                typename SIter::difference_type>::value,
                "RIter and SIter difference type must be the same");

  typedef typename RIter::difference_type distance_type;
  typedef typename RIter::value_type::first_type Key;
  typedef typename RIter::value_type::second_type RValue;
  typedef typename SIter::value_type::second_type SValue;
  typedef typename std::tuple<std::size_t, Key, RValue> RTuple;
  typedef typename std::tuple<std::size_t, Key, SValue> STuple;
  typedef typename std::vector<RTuple>::iterator RSortedIter;
  typedef typename std::vector<STuple>::iterator SSortedIter;

  //protected:
 public:
  HashMergeJoin() = default;
  HashMergeJoin(RIter r_begin, RIter r_end,
                SIter s_begin, SIter s_end,
                unsigned int num_threads = 1)  {
    distance_type r_size, s_size;
    r_size = std::distance(r_begin, r_end);
    s_size = std::distance(s_begin, s_end);
    _r_sorted = std::vector<std::tuple<std::size_t, Key, RValue>>(r_size);
    _s_sorted = std::vector<std::tuple<std::size_t, Key, RValue>>(s_size);

    ::radix_hash_bf6<Key, RValue>(r_begin, r_end, _r_sorted.begin(),
                                  11, 0, num_threads);

    ::radix_hash_bf6<Key, SValue>(s_begin, s_end, _s_sorted.begin(),
                                  11, 0, num_threads);
  }

  class iterator : std::iterator<std::input_iterator_tag,
    std::tuple<Key*, RValue*, SValue*>> {
  public:
  iterator(RSortedIter rs_iter, RSortedIter rs_end,
           SSortedIter ss_iter, SSortedIter ss_end)
    : _rs_iter(rs_iter), _rs_end(rs_end),
      _ss_iter(ss_iter), _ss_end(ss_end) {
      while (true) {
        if (_rs_iter == _rs_end) {
          _ss_iter = _ss_end;
          break;
        }
        if (_ss_iter == _ss_end) {
          _rs_iter = _rs_end;
          break;
        }
        if (std::get<0>(*_rs_iter) < std::get<0>(*_ss_iter)) {
          _rs_iter++;
          continue;
        }
        if (std::get<0>(*_ss_iter) < std::get<0>(*_rs_iter)) {
          _ss_iter++;
          continue;
        }
        if (std::get<1>(*_rs_iter) == std::get<1>(*_ss_iter)) {
          break;
        }
        if (std::get<1>(*_rs_iter) < std::get<1>(*_ss_iter)) {
          _rs_iter++;
          continue;
        }
        _ss_iter++;
      }
    }
    iterator& operator++() {
      if (_rs_iter+1 == _rs_end) {
        _rs_iter = _rs_end;
        _ss_iter = _ss_end;
        return *this;
      }
      if (_ss_iter+1 == _ss_end) {
        _rs_iter = _rs_end;
        _ss_iter = _ss_end;
        return *this;
      }
      if (std::get<0>(*_rs_iter) == std::get<0>(*(_rs_iter+1))) {
        _rs_iter++;
        goto find_match;
      }
      if (std::get<0>(*_ss_iter) == std::get<0>(*(_ss_iter+1))) {
        _ss_iter++;
        goto find_match;
      }
      _rs_iter++;
      _ss_iter++;

    find_match:
      while (true) {
        if (_rs_iter == _rs_end) {
          _ss_iter = _ss_end;
          break;
        }
        if (_ss_iter == _ss_end) {
          _rs_iter = _rs_end;
          break;
        }
        if (std::get<0>(*_rs_iter) < std::get<0>(*_ss_iter)) {
          _rs_iter++;
          continue;
        }
        if (std::get<0>(*_ss_iter) < std::get<0>(*_rs_iter)) {
          _ss_iter++;
          continue;
        }
        if (std::get<1>(*_rs_iter) == std::get<1>(*_ss_iter)) {
          break;
        }
        if (std::get<1>(*_rs_iter) < std::get<1>(*_ss_iter)) {
          _rs_iter++;
          continue;
        }
        _ss_iter++;
      }
      return *this;
    }
    iterator operator++(int) {
      iterator retval = *this;
      ++(*this);
      return retval;
    }
    bool operator==(iterator other) const {
      return _rs_iter == other._rs_iter &&
      _ss_iter == other._ss_iter;
      //_ss_dupcnt == other._ss_dupcnt;
    }
    bool operator!=(iterator other) const {
      return _rs_iter != other._rs_iter || _ss_iter != other._ss_iter;
    }
    std::tuple<Key*, RValue*, SValue*>& operator*() {
      tmp_val = std::make_tuple(&std::get<1>(*_rs_iter),
                                &std::get<2>(*_rs_iter),
                                &std::get<2>(*_ss_iter));
      return tmp_val;
    }
  protected:
    RSortedIter _rs_iter;
    RSortedIter _rs_end;
    SSortedIter _ss_iter;
    SSortedIter _ss_end;
    std::tuple<Key*, RValue*, SValue*> tmp_val;
  };

 public:
  iterator begin() {
    return iterator(_r_sorted.begin(), _r_sorted.end(),
                    _s_sorted.begin(), _s_sorted.end());
  }

  iterator end() {
    return iterator(_r_sorted.end(), _r_sorted.end(),
                    _s_sorted.end(), _s_sorted.end());
  }
  void clear() {
    _r_sorted.clear();
    _s_sorted.clear();
  }
 protected:
  std::vector<std::tuple<std::size_t, Key, RValue>> _r_sorted;
  std::vector<std::tuple<std::size_t, Key, SValue>> _s_sorted;
};

template<typename RIter, typename SIter>
class HashMergeJoin2 {
  static_assert(std::is_same<
                typename std::tuple_element
                <1, typename RIter::value_type>::type,
                typename std::tuple_element
                <1, typename SIter::value_type>::type>::value,
                "RIter and SIter key type must be the same");
  static_assert(std::is_same<
                typename RIter::difference_type,
                typename SIter::difference_type>::value,
                "RIter and SIter difference type must be the same");

  typedef typename RIter::difference_type distance_type;
  typedef typename std::tuple_element<1, typename RIter::value_type>::type Key;
  typedef typename std::tuple_element
    <2, typename RIter::value_type>::type RValue;
  typedef typename std::tuple_element
    <2, typename SIter::value_type>::type SValue;

  //protected:
 public:
  HashMergeJoin2() = default;
  HashMergeJoin2(RIter r_begin, RIter r_end,
                 SIter s_begin, SIter s_end,
                 unsigned int num_threads = 1)
    : _r_begin(r_begin), _r_end(r_end),
    _s_begin(s_begin), _s_end(s_end) {
    distance_type r_size, s_size;

    r_size = std::distance(r_begin, r_end);
    s_size = std::distance(s_begin, s_end);

    ::radix_hash_bf8(_r_begin, r_size, 11, num_threads);
    ::radix_hash_bf8(_s_begin, s_size, 11, num_threads);
  }

  class iterator : std::iterator<std::input_iterator_tag,
    std::tuple<Key*, RValue*, SValue*>> {
  public:
  iterator(RIter rs_iter, RIter rs_end,
           SIter ss_iter, SIter ss_end)
    : _rs_iter(rs_iter), _rs_end(rs_end),
      _ss_iter(ss_iter), _ss_end(ss_end) {
      while (true) {
        if (_rs_iter == _rs_end) {
          _ss_iter = _ss_end;
          break;
        }
        if (_ss_iter == _ss_end) {
          _rs_iter = _rs_end;
          break;
        }
        if (std::get<0>(*_rs_iter) < std::get<0>(*_ss_iter)) {
          _rs_iter++;
          continue;
        }
        if (std::get<0>(*_ss_iter) < std::get<0>(*_rs_iter)) {
          _ss_iter++;
          continue;
        }
        if (std::get<1>(*_rs_iter) == std::get<1>(*_ss_iter)) {
          break;
        }
        if (std::get<1>(*_rs_iter) < std::get<1>(*_ss_iter)) {
          _rs_iter++;
          continue;
        }
        _ss_iter++;
      }
    }
    iterator& operator++() {
      if (_rs_iter+1 == _rs_end) {
        _rs_iter = _rs_end;
        _ss_iter = _ss_end;
        return *this;
      }
      if (_ss_iter+1 == _ss_end) {
        _rs_iter = _rs_end;
        _ss_iter = _ss_end;
        return *this;
      }
      if (std::get<0>(*_rs_iter) == std::get<0>(*(_rs_iter+1))) {
        _rs_iter++;
        goto find_match;
      }
      if (std::get<0>(*_ss_iter) == std::get<0>(*(_ss_iter+1))) {
        _ss_iter++;
        goto find_match;
      }
      _rs_iter++;
      _ss_iter++;

    find_match:
      while (true) {
        if (_rs_iter == _rs_end) {
          _ss_iter = _ss_end;
          break;
        }
        if (_ss_iter == _ss_end) {
          _rs_iter = _rs_end;
          break;
        }
        if (std::get<0>(*_rs_iter) < std::get<0>(*_ss_iter)) {
          _rs_iter++;
          continue;
        }
        if (std::get<0>(*_ss_iter) < std::get<0>(*_rs_iter)) {
          _ss_iter++;
          continue;
        }
        if (std::get<1>(*_rs_iter) == std::get<1>(*_ss_iter)) {
          break;
        }
        if (std::get<1>(*_rs_iter) < std::get<1>(*_ss_iter)) {
          _rs_iter++;
          continue;
        }
        _ss_iter++;
      }
      return *this;
    }
    iterator operator++(int) {
      iterator retval = *this;
      ++(*this);
      return retval;
    }
    bool operator==(iterator other) const {
      return _rs_iter == other._rs_iter &&
      _ss_iter == other._ss_iter;
      //_ss_dupcnt == other._ss_dupcnt;
    }
    bool operator!=(iterator other) const {
      return _rs_iter != other._rs_iter || _ss_iter != other._ss_iter;
    }
    std::tuple<Key*, RValue*, SValue*>& operator*() {
      tmp_val = std::make_tuple(&std::get<1>(*_rs_iter),
                                &std::get<2>(*_rs_iter),
                                &std::get<2>(*_ss_iter));
      return tmp_val;
    }
  protected:
    RIter _rs_iter;
    RIter _rs_end;
    SIter _ss_iter;
    SIter _ss_end;
    std::tuple<Key*, RValue*, SValue*> tmp_val;
  };

 public:
  iterator begin() {
    return iterator(_r_begin, _r_end, _s_begin, _s_end);
  }

  iterator end() {
    return iterator(_r_end, _r_end, _s_end, _s_end);
  }
 protected:
  RIter _r_begin;
  RIter _r_end;
  SIter _s_begin;
  SIter _s_end;
};
#endif
