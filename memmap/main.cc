
#include "benchmark.h"

#include <cstdlib>
#include <iostream>

int main()
{
#ifdef USE_MEMMAP
  eds_memmap_initialize();
#endif

  int_vector_type vector;

  std::cout << "Using " << vector_name << "<int>\n"
    "capacity : " << vector.capacity() << "\n";
  vector.push_back(1234);
  std::cout << "size : " << vector.size() << "\n";
  std::cout << "capacity : " << vector.capacity() << "\n";
  for (int n = 0; n < 0x3333; ++n) {
    vector.push_back(n);
  }
  std::cout << "size : " << vector.size() << "\n";
  std::cout << "capacity : " << vector.capacity() << "\n";
  for (int n = 0; n < 0x4321; ++n) {
    vector.push_back(n);
  }
  std::cout << "size : " << vector.size() << "\n";
  std::cout << "capacity : " << vector.capacity() << "\n";

  loop_stress_vector();

  std::cout << "Done using " << vector_name << "<int>\n";

  return EXIT_SUCCESS;
}
