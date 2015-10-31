
#ifndef BENCHMARK_H
#define BENCHMARK_H

#ifdef USE_MEMMAP

#include "memmap.h"
typedef eds::memmap<int> int_vector_type;
static constexpr char vector_name[] = "mremap_vector";

#elif defined(USE_REALLOC_VECTOR)

#include "realloc_vector.h"
typedef eds::realloc_vector<int> int_vector_type;
static constexpr char vector_name[] = "realloc_vector";

#else

#include <vector>
typedef ::std::vector<int> int_vector_type;
static constexpr char vector_name[] = "std::vector";

#endif

void stress_vector(int_vector_type&);
void loop_stress_vector();

#endif /* BENCHMARK_H */
