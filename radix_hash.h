#ifndef RADIX_HASH_H
#define RADIX_HASH_H 1

  /*
class IndexIter: public std::iterator<
  std::input_iterator_tag,
  std::pair<int, int[]>>
{
  typedef void (IndexIter::*BoolLike)();
  void non_comparable();
 public:
  using iterator_category = std::input_iterator_tag;
  using value_type = std::pair<int, int[]>;
  using reference = value_type const&;
  using pointer = value_type const*;
  using difference_type = ptrdiff_t;

  typedef std::input_iterator_tag iterator_category;
  typedef std::pair<unsigned, unsigned> value_type;
  typedef value_type const& reference;
  typedef value_type const* pointer;
  typedef ptrdiff_t difference_type;

 IndexIter(): done(false) {}

  explicit operator bool() const {return !done; }

  operator BoolLike() const {
    return done ? 0 : &IndexIter::non_comparable;
  }

  reference operator*() const { return mypair; }
  pointer operator->() const { return &mypair; }

  IndexIter& operator++() {
    return *this;
  }

  IndexIter operator++(int) {

  }
}
  */

#endif
