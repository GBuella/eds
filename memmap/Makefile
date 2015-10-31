
CXX_FLAGS ?= -std=c++11 -O2 -DNDEBUG -march=native -Wall -Wextra -pedantic -Werror
CC_FLAGS ?= -std=c99 -O2 -DNDEBUG -march=native -Wall -Wextra -pedantic -Werror

# CXX_FLAGS ?= -std=c++11 -O0 -g -march=native -Wall -Wextra -pedantic
# CC_FLAGS ?= -std=c99 -O0 -g -march=native -Wall -Wextra -pedantic

all: test_realloc_vector test_std_vector test_memmap

BENCHMARK_SRCS=main.cc stress_vector.cc loop_stress_vector.cc

test_realloc_vector: realloc_vector.h $(BENCHMARK_SRCS)
	$(CXX) $(CXX_FLAGS) -DUSE_REALLOC_VECTOR $(BENCHMARK_SRCS) -o $@

test_std_vector: $(BENCHMARK_SRCS)
	$(CXX) $(CXX_FLAGS) $(BENCHMARK_SRCS) -o $@

libeds_memmap.so: eds_memmap.c eds_memmap.h
	$(CC) $(CC_FLAGS) eds_memmap.c -shared -fPIC -o $@

test_memmap: memmap.h mapped_storage.h eds_memmap.h libeds_memmap.so $(BENCHMARK_SRCS)
	$(CXX) $(CXX_FLAGS) -DUSE_MEMMAP $(BENCHMARK_SRCS) ./libeds_memmap.so -o $@

clean:
	$(RM) test_std_vector test_realloc_vector test_memmap libeds_memmap.so

