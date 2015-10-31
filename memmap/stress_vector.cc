
#include "benchmark.h"

#include <iostream>

void stress_vector(int_vector_type& vector)
{
  vector.push_back(54321);
  vector.resize(vector.size() * 3, 77);
  vector.resize(vector.size() * 3, 99);
  vector.resize(vector.size() * 5, 99);
#ifdef USE_MEMMAP
  vector.push_front(34);
#endif
  vector.resize(vector.size() * 2, 199);
  vector.resize(vector.size() * 11, 44);
  vector.resize(vector.size() * 3, 1234);
#ifdef USE_MEMMAP
  vector.push_front(56);
#endif
  for (int n = 0; n < 12; ++n) {
    vector.resize(vector.size() * 2, 5678 + n);
  }
}

