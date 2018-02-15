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

void single_table_join(KeyValVec r, KeyValVec s,
    std::function<void(std::string const& key, uint64_t r_val, uint64_t s_val)>
    callback);

void radix_hash_table_join(KeyValVec r, KeyValVec s,
    std::function<void(std::string const& key, uint64_t r_val, uint64_t s_val)>
    callback);

void radix_hash_join(KeyValVec r, KeyValVec s,
    std::function<void(std::string const& key, uint64_t r_val, uint64_t s_val)>
    callback);

template<typename F>
void radix_hash_join_templated(KeyValVec r, KeyValVec s, F const& callback) {
  auto r_sorted = HashKeyValVec(r.size());
  auto s_sorted = HashKeyValVec(s.size());

  ::radix_hash_bf1<std::string, uint64_t>(r.begin(), r.end(), r_sorted.begin(),
                                          ::compute_power(r.size()), 11, 0);
  ::radix_hash_bf1<std::string, uint64_t>(s.begin(), s.end(), s_sorted.begin(),
                                          ::compute_power(s.size()), 11, 0);

  for (auto r_it = r_sorted.begin(),
         s_it = s_sorted.begin();
       r_it != r_sorted.end() || s_it != s_sorted.end();) {
    callback(std::get<1>(*r_it), std::get<2>(*r_it), std::get<2>(*s_it));
    r_it++;
    s_it++;
  }
}

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
 public:
  HashMergeJoin(RIter r_begin, RIter r_end,
                SIter s_begin, SIter s_end) {
    distance_type r_size, s_size, max_size;
    int mask_bits;
    r_size = std::distance(r_begin, r_end);
    s_size = std::distance(s_begin, s_end);
    //std::cout << "r_size: " << r_size << "\n";
    max_size = std::max(r_size, s_size);
    mask_bits = ::compute_power(max_size);

    r_sorted = std::vector<std::tuple<std::size_t, Key, RValue>>(r_size);
    s_sorted = std::vector<std::tuple<std::size_t, Key, SValue>>(s_size);

    ::radix_hash_bf1<Key, RValue>(r_begin, r_end, r_sorted.begin(),
                                 mask_bits, 11, 0);
    ::radix_hash_bf1<Key, SValue>(s_begin, s_end, s_sorted.begin(),
                                 mask_bits, 11, 0);
    _mask = (1ULL << mask_bits) - 1;
  }

  class iterator : std::iterator<std::input_iterator_tag,
    std::tuple<Key*, RValue*, SValue*>> {
  public:
  iterator(std::size_t mask,
           RSortedIter rs_iter, RSortedIter rs_end,
           SSortedIter ss_iter, SSortedIter ss_end):
    _mask(mask), _rs_iter(rs_iter), _rs_end(rs_end),
      _ss_iter(ss_iter), _ss_end(ss_end) {
      std::size_t r_hash, s_hash;
      while (_rs_iter != _rs_end) {
        if (_ss_iter == _ss_end) {
          _rs_iter = _rs_end;
          break;
        }
        r_hash = std::get<0>(*_rs_iter);
        s_hash = std::get<0>(*_ss_iter);
        if ((s_hash & _mask) < (r_hash & _mask)) {
          _ss_iter++;
          continue;
        }
        if ((s_hash & _mask) > (r_hash & _mask)) {
          _rs_iter++;
          continue;
        }
        if (std::get<1>(*_ss_iter) == std::get<1>(*_rs_iter)) {
          break;
        }
        if (s_hash < r_hash) {
          _ss_iter++;
          continue;
        }
        if (s_hash > r_hash) {
          _rs_iter++;
          continue;
        }
        if (std::get<1>(*_ss_iter) < std::get<1>(*_rs_iter)) {
          _ss_iter++;
          continue;
        }
        if (std::get<1>(*_ss_iter) > std::get<1>(*_rs_iter)) {
          _rs_iter++;
          continue;
        }
        break;
      }
    }
    iterator& operator++() {
      std::size_t r_hash, s_hash, s_hash_next;
      if (_ss_iter + 1 == _ss_end) {
        if (_ss_dupcnt > 0) {
          if (_rs_iter + 1 == _rs_end) {
            _rs_iter = _rs_end;
            _ss_iter = _ss_end;
            _ss_dupcnt = 0;
            return *this;
          }
          ++_rs_iter;
          _ss_iter -= _ss_dupcnt;
          _ss_dupcnt = 0;
          return *this;
        }
        _rs_iter = _rs_end;
        _ss_iter = _ss_end;
        _ss_dupcnt = 0;
        return *this;
      }
      r_hash = std::get<0>(*_rs_iter);
      s_hash = std::get<0>(*_ss_iter);
      s_hash_next = std::get<0>(*(_ss_iter+1));

      if (s_hash == s_hash_next &&
          std::get<1>(*_ss_iter) == std::get<1>(*(_ss_iter + 1))) {
        ++_ss_dupcnt;
        ++_ss_iter;
        return *this;
      }
      if (_ss_dupcnt > 0) {
        if (_rs_iter + 1 == _rs_end) {
          _rs_iter = _rs_end;
          _ss_iter = _ss_end;
          _ss_dupcnt = 0;
          return *this;
        }
        if (std::get<1>(*_rs_iter) == std::get<1>(*(_rs_iter+1))) {
          _ss_iter -= _ss_dupcnt;
          ++_rs_iter;
          _ss_dupcnt = 0;
          return *this;
        }
        _ss_dupcnt = 0;
      }
      ++_rs_iter;
      ++_ss_iter;
      while (_rs_iter != _rs_end) {
        if (_ss_iter == _ss_end) {
          _rs_iter = _rs_end;
          break;
        }
        r_hash = std::get<0>(*_rs_iter);
        s_hash = std::get<0>(*_ss_iter);
        if ((s_hash & _mask) < (r_hash & _mask)) {
          _ss_iter++;
          continue;
        }
        if ((s_hash & _mask) > (r_hash & _mask)) {
          _rs_iter++;
          continue;
        }
        if (std::get<1>(*_ss_iter) == std::get<1>(*_rs_iter)) {
          break;
        }
        if (s_hash < r_hash) {
          _ss_iter++;
          continue;
        }
        if (s_hash > r_hash) {
          _rs_iter++;
          continue;
        }
        if (std::get<1>(*_ss_iter) < std::get<1>(*_rs_iter)) {
          _ss_iter++;
          continue;
        }
        if (std::get<1>(*_ss_iter) > std::get<1>(*_rs_iter)) {
          _rs_iter++;
          continue;
        }
        break;
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
      _ss_iter == other._ss_iter &&
      _ss_dupcnt == other._ss_dupcnt;
    }
    bool operator!=(iterator other) const {
      return !(*this == other);
    }
    std::tuple<Key*, RValue*, SValue*>& operator*() {
      tmp_val = std::make_tuple(&std::get<1>(*_rs_iter),
                                &std::get<2>(*_rs_iter),
                                &std::get<2>(*_ss_iter));
      return tmp_val;
    }
  private:
    std::size_t _mask;
    RSortedIter _rs_iter;
    RSortedIter _rs_end;
    SSortedIter _ss_iter;
    SSortedIter _ss_end;
    int _ss_dupcnt = 0;
    std::tuple<Key*, RValue*, SValue*> tmp_val;
  };

  iterator begin() {
    return iterator(_mask,
                    r_sorted.begin(), r_sorted.end(),
                    s_sorted.begin(), s_sorted.end());
  }

  iterator end() {
    return iterator(_mask,
                    r_sorted.end(), r_sorted.end(),
                    s_sorted.end(), s_sorted.end());
  }
 private:
  std::size_t _mask;
  std::vector<std::tuple<std::size_t, Key, RValue>> r_sorted;
  std::vector<std::tuple<std::size_t, Key, SValue>> s_sorted;
};

template<typename RIter, typename SIter>
class HashMergeJoinSimpleThread {
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
 public:
  HashMergeJoinSimpleThread(RIter r_begin, RIter r_end,
                            SIter s_begin, SIter s_end) {
    distance_type r_size, s_size, max_size;
    int mask_bits;
    r_size = std::distance(r_begin, r_end);
    s_size = std::distance(s_begin, s_end);
    //std::cout << "r_size: " << r_size << "\n";
    max_size = std::max(r_size, s_size);
    mask_bits = ::compute_power(max_size);

    r_sorted = std::vector<std::tuple<std::size_t, Key, RValue>>(r_size);
    s_sorted = std::vector<std::tuple<std::size_t, Key, SValue>>(s_size);

    std::thread r_thread(::radix_hash_bf1<Key, RValue,
                         std::hash<Key>, RIter, RSortedIter>,
                         r_begin, r_end, r_sorted.begin(),
                         mask_bits, 11, 0);
    std::thread s_thread(::radix_hash_bf1<Key, SValue,
                         std::hash<Key>, SIter, SSortedIter>,
                         s_begin, s_end, s_sorted.begin(),
                         mask_bits, 11, 0);
    _mask = (1ULL << mask_bits) - 1;
    r_thread.join();
    s_thread.join();
  }

  class iterator : std::iterator<std::input_iterator_tag,
    std::tuple<Key*, RValue*, SValue*>> {
  public:
  iterator(std::size_t mask,
           RSortedIter rs_iter, RSortedIter rs_end,
           SSortedIter ss_iter, SSortedIter ss_end):
    _mask(mask), _rs_iter(rs_iter), _rs_end(rs_end),
      _ss_iter(ss_iter), _ss_end(ss_end) {
      std::size_t r_hash, s_hash;
      while (_rs_iter != _rs_end) {
        if (_ss_iter == _ss_end) {
          _rs_iter = _rs_end;
          break;
        }
        r_hash = std::get<0>(*_rs_iter);
        s_hash = std::get<0>(*_ss_iter);
        if ((s_hash & _mask) < (r_hash & _mask)) {
          _ss_iter++;
          continue;
        }
        if ((s_hash & _mask) > (r_hash & _mask)) {
          _rs_iter++;
          continue;
        }
        if (std::get<1>(*_ss_iter) == std::get<1>(*_rs_iter)) {
          break;
        }
        if (s_hash < r_hash) {
          _ss_iter++;
          continue;
        }
        if (s_hash > r_hash) {
          _rs_iter++;
          continue;
        }
        if (std::get<1>(*_ss_iter) < std::get<1>(*_rs_iter)) {
          _ss_iter++;
          continue;
        }
        if (std::get<1>(*_ss_iter) > std::get<1>(*_rs_iter)) {
          _rs_iter++;
          continue;
        }
        break;
      }
    }
    iterator& operator++() {
      std::size_t r_hash, s_hash, s_hash_next;
      if (_ss_iter + 1 == _ss_end) {
        if (_ss_dupcnt > 0) {
          if (_rs_iter + 1 == _rs_end) {
            _rs_iter = _rs_end;
            _ss_iter = _ss_end;
            _ss_dupcnt = 0;
            return *this;
          }
          ++_rs_iter;
          _ss_iter -= _ss_dupcnt;
          _ss_dupcnt = 0;
          return *this;
        }
        _rs_iter = _rs_end;
        _ss_iter = _ss_end;
        _ss_dupcnt = 0;
        return *this;
      }
      r_hash = std::get<0>(*_rs_iter);
      s_hash = std::get<0>(*_ss_iter);
      s_hash_next = std::get<0>(*(_ss_iter+1));

      if (s_hash == s_hash_next &&
          std::get<1>(*_ss_iter) == std::get<1>(*(_ss_iter + 1))) {
        ++_ss_dupcnt;
        ++_ss_iter;
        return *this;
      }
      if (_ss_dupcnt > 0) {
        if (_rs_iter + 1 == _rs_end) {
          _rs_iter = _rs_end;
          _ss_iter = _ss_end;
          _ss_dupcnt = 0;
          return *this;
        }
        if (std::get<1>(*_rs_iter) == std::get<1>(*(_rs_iter+1))) {
          _ss_iter -= _ss_dupcnt;
          ++_rs_iter;
          _ss_dupcnt = 0;
          return *this;
        }
        _ss_dupcnt = 0;
      }
      ++_rs_iter;
      ++_ss_iter;
      while (_rs_iter != _rs_end) {
        if (_ss_iter == _ss_end) {
          _rs_iter = _rs_end;
          break;
        }
        r_hash = std::get<0>(*_rs_iter);
        s_hash = std::get<0>(*_ss_iter);
        if ((s_hash & _mask) < (r_hash & _mask)) {
          _ss_iter++;
          continue;
        }
        if ((s_hash & _mask) > (r_hash & _mask)) {
          _rs_iter++;
          continue;
        }
        if (std::get<1>(*_ss_iter) == std::get<1>(*_rs_iter)) {
          break;
        }
        if (s_hash < r_hash) {
          _ss_iter++;
          continue;
        }
        if (s_hash > r_hash) {
          _rs_iter++;
          continue;
        }
        if (std::get<1>(*_ss_iter) < std::get<1>(*_rs_iter)) {
          _ss_iter++;
          continue;
        }
        if (std::get<1>(*_ss_iter) > std::get<1>(*_rs_iter)) {
          _rs_iter++;
          continue;
        }
        break;
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
      _ss_iter == other._ss_iter &&
      _ss_dupcnt == other._ss_dupcnt;
    }
    bool operator!=(iterator other) const {
      return !(*this == other);
    }
    std::tuple<Key*, RValue*, SValue*>& operator*() {
      tmp_val = std::make_tuple(&std::get<1>(*_rs_iter),
                                &std::get<2>(*_rs_iter),
                                &std::get<2>(*_ss_iter));
      return tmp_val;
    }
  private:
    std::size_t _mask;
    RSortedIter _rs_iter;
    RSortedIter _rs_end;
    SSortedIter _ss_iter;
    SSortedIter _ss_end;
    int _ss_dupcnt = 0;
    std::tuple<Key*, RValue*, SValue*> tmp_val;
  };

  iterator begin() {
    return iterator(_mask,
                    r_sorted.begin(), r_sorted.end(),
                    s_sorted.begin(), s_sorted.end());
  }

  iterator end() {
    return iterator(_mask,
                    r_sorted.end(), r_sorted.end(),
                    s_sorted.end(), s_sorted.end());
  }
 private:
  std::size_t _mask;
  std::vector<std::tuple<std::size_t, Key, RValue>> r_sorted;
  std::vector<std::tuple<std::size_t, Key, SValue>> s_sorted;
};

#endif
