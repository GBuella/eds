
#ifndef EDS_MEMMAP_BASE_H
#define EDS_MEMMAP_BASE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

void eds_memmap_initialize(void);

char *eds_memmap_create(size_t);

char *eds_memmap_expand_high(char* mem, size_t size, size_t delta);
char *eds_memmap_expand_low(char* mem, size_t size, size_t delta);
char *eds_memmap_expand(char* mem, size_t size,
                        size_t delta_high, size_t delta_low);

char *eds_memmap_shrink_high(char* mem, size_t size, size_t delta);
char *eds_memmap_shrink_low(char* mem, size_t size, size_t delta);
char *eds_memmap_shrink(char* mem, size_t size,
                        size_t delta_high, size_t delta_low);

#ifdef __cplusplus
char *eds_memmap_merge(char* mem_a, size_t size_a,
                       char* mem_b, size_t size_b);
#else
char *eds_memmap_merge(char* restrict mem_a, size_t size_a,
                       char* restrict mem_b, size_t size_b);
#endif

char *eds_memmap_split(char* mem, size_t size, size_t cut);

void eds_memmap_destroy(char* mem, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* EDS_MEMMAP_BASE_H */
