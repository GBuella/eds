
#include "benchmark.h"

#include <iostream>

void loop_stress_vector()
{
  for (int n = 0; n < 0xff; ++n) {
    int_vector_type fresh_vector;

    stress_vector(fresh_vector);

//    std::cout << "size : " << fresh_vector.size() << "\n";
 //   std::cout << "capacity : " << fresh_vector.capacity() << "\n";
  }
}

