
#ifndef REALLOC_VECTOR_H
#define REALLOC_VECTOR_H

#include <cstddef>
#include <cstdlib>
#include <limits>
#include <new>

namespace eds
{

template<typename type>
class realloc_vector
{
private:

  type* raw_data;
  size_t allocated_count;
  size_t raw_count;

  static constexpr size_t max_count = 
    std::numeric_limits<size_t>::max() / sizeof(*raw_data);

  size_t get_new_size(size_t required_increase)
  {
    static constexpr size_t growth_rate = 2;

    size_t new_count = allocated_count + required_increase;

    if (new_count < allocated_count * growth_rate) {
      if (allocated_count * growth_rate <= max_count) {
        new_count = allocated_count * growth_rate;
      }
    }
    else if (new_count < allocated_count) {
      throw std::bad_alloc();
    }
    else if (new_count > max_count) {
      throw std::bad_alloc();
    }
    return new_count;
  }

  void grow(size_t required_increase)
  {
    size_t new_count = get_new_size(required_increase);
    void* new_pointer = std::realloc(raw_data, new_count * sizeof(*raw_data));

    if (new_pointer == nullptr) {
      throw std::bad_alloc();
    }
    raw_data = static_cast<type*>(new_pointer);
    allocated_count = new_count;
  }

public:

  realloc_vector():
    raw_data(nullptr),
    allocated_count(0),
    raw_count(0)
  {
  }

  size_t capacity() const noexcept
  {
    return allocated_count;
  }

  size_t size() const noexcept
  {
    return raw_count;
  }

  void push_back(const type& value)
  {
    if (raw_count == allocated_count) {
      grow(1);
    }
    raw_data[raw_count] = value;
    ++raw_count;
  }

  size_t max_size() const noexcept
  {
    return max_count;
  }

  type* data() const noexcept
  {
    return raw_data;
  }

  void resize(size_t count, const type& value)
  {
    if (count <= raw_count) {
      raw_count = count;
    }
    if (count > allocated_count) {
      grow(count - allocated_count);
    }
    for (size_t index = raw_count; index < count; ++index) {
      raw_data[index] = value;
    }
    raw_count = count;
  }

  ~realloc_vector()
  {
    std::free(raw_data);
  }

};

}

#endif /* REALLOC_VECTOR_H */
