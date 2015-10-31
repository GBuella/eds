
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "eds_memmap.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <stdint.h>
#include <sys/mman.h>

#ifndef RSIZE_MAX
#define RSIZE_MAX (SIZE_MAX / 2)
#endif

/* all allocation with size below mmap_treshold
   are just forwarded to libc malloc/free
*/
#ifdef EDS_MMAP_TRESHOLD
static const size_t mmap_treshold = EDS_MMAP_TRESHOLD;
#else
static const size_t mmap_treshold = 0x20000;
#endif


/* Forgetting to call eds_memmap_initialize leaves page_size at zero,
   which results in division-by-zero errors in other eds_* calls.
  */
static size_t page_size;
static size_t page_mask;

static bool is_power_of_two(size_t value)
{
    return (value & ~(value - 1)) == value;
}

void eds_memmap_initialize(void)
{
    long sysconf_result;

    sysconf_result = sysconf(_SC_PAGESIZE);
    if (sysconf_result < 1 || (size_t)sysconf_result > RSIZE_MAX) {
        abort();
    }
    page_size = (size_t)sysconf_result;
    if (!is_power_of_two(page_size)) {
        abort();
    }
    page_mask = ~(page_size - 1);
}

static ptrdiff_t
in_page_offset(char* address)
{
    assert(page_size != 0);
    return ((uintptr_t)address) % page_size;
}

static char*
page_boundary(char* address)
{
    assert(page_mask != 0);
    return (char*)(((uintptr_t)address) & page_mask);
}

static size_t
round_up(size_t size)
{
    assert(page_size != 0 && page_mask != 0);

    if ((size & page_mask) == size) {
        return size;
    }
    else {
        return (size & page_mask) + page_size;
    }
}

static size_t
total_size(char* mem, size_t size)
{
    return round_up(size + in_page_offset(mem));
}

static char*
mmap_wrapper(size_t size)
{
    char *new_address;

    new_address = mmap(NULL, round_up(size),
                       PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (new_address == MAP_FAILED) {
        new_address = NULL;
    }
    return new_address;
}

static void
munmap_wrapper(char* mem, size_t size)
{
    int munmap_result;

    if (size == 0) {
        return;
    }
    assert(mem != NULL);
    munmap_result = munmap(page_boundary(mem), size);
    assert(munmap_result == 0);
    if (munmap_result != 0) {
        abort();
    }
    (void)munmap_result;
}

char* eds_memmap_create(size_t size)
{
    if (size == 0 || size > RSIZE_MAX) {
        return NULL;
    }
    else if (size < mmap_treshold) {
        return malloc(size);
    }
    else {
        return mmap_wrapper(size);
    }
}

void eds_memmap_destroy(char* mem, size_t size)
{
    assert((mem == NULL && size == 0) || (mem != NULL && size != 0));

    if (mem == NULL || size == 0) {
        return;
    }
    else if (size < mmap_treshold) {
        free(mem);
    }
    else {
        munmap_wrapper(mem, total_size(mem, size));
    }
}

static size_t
capacity_high(char* mem, size_t size)
{
    size_t page_mod = (size + in_page_offset(mem)) % page_size;

    return (page_size - page_mod) % page_size;
}

static size_t
capacity_low(char* mem)
{
    return in_page_offset(mem);
}

static char*
expand_small_high_address(char* mem, size_t size, size_t delta)
{
    assert(mem != NULL);
    assert(size > 0 && size < mmap_treshold);
    assert(delta > 0);

    if (size + delta < mmap_treshold) {
        return realloc(mem, size + delta);
    }
    else {
        char *new_address;

        new_address = mmap_wrapper(size + delta);
        if (new_address == NULL) {
            return NULL;
        }
        memcpy(new_address, mem, size);
        free(mem);
        return new_address;
    }
}

static char*
expand_large_high_address(char* mem, size_t size, size_t delta)
{
    assert(mem != NULL);
    assert(size >= mmap_treshold);
    assert(delta > 0);

    if (capacity_high(mem, size) >= delta) {
        return mem;
    }
    else {
        char *new_address;

        new_address = mremap(page_boundary(mem),
                total_size(mem, size),
                total_size(mem, size + delta),
                MREMAP_MAYMOVE);
        if (new_address != MAP_FAILED) {
            return new_address;
        }
        else {
            return NULL;
        }
    }
}

char* eds_memmap_expand_high(char* mem, size_t size, size_t delta)
{
    assert((mem == NULL && size == 0) || (mem != NULL && size != 0));
    assert(size <= RSIZE_MAX);

    if (delta == 0) {
        return mem;
    }
    else if ((size + delta) < size || (size + delta) > RSIZE_MAX) {
        return NULL;
    }
    else if (mem == NULL) {
        return eds_memmap_create(delta);
    }
    else if (size < mmap_treshold) {
        return expand_small_high_address(mem, size, delta);
    }
    else {
        return expand_large_high_address(mem, size, delta);
    }
}

static char*
expand_both_ends_small(char* mem, size_t size,
                       size_t delta_high, size_t delta_low)
{
    assert(mem != NULL);
    assert(size > 0 && size < mmap_treshold);
    assert(delta_low > 0);

    char* new_address;
    if (size + delta_low + delta_high < mmap_treshold) {
        new_address = malloc(size + delta_low + delta_high);
    }
    else {
        new_address = mmap_wrapper(size + delta_low + delta_high);
    }
    if (new_address == NULL) {
        return NULL;
    }
    memcpy(new_address + delta_low, mem, size);
    free(mem);
    return new_address;
}

static char*
expand_both_ends_large(char* mem, size_t size,
                       size_t delta_high, size_t delta_low)
{
    assert(mem != NULL);
    assert(size >= mmap_treshold);
    assert(delta_low > 0);

    if (capacity_low(mem) >= delta_low &&
        capacity_high(mem, size) >= delta_high)
    {
        return mem - delta_low;
    }
    else {
        char* new_address;
        void* remap_result;
        size_t new_low_pages;
        size_t old_size;
        size_t new_size;

        old_size = total_size(mem, size);
        new_low_pages = round_up(delta_low - in_page_offset(mem));
        new_size = old_size + new_low_pages;
        if (delta_high > capacity_high(mem, size)) {
            new_size += round_up(delta_high - capacity_high(mem, size));
        }
        new_address = mmap_wrapper(new_size);
        if (new_address == NULL) {
            return NULL;
        }
        remap_result = mremap(page_boundary(mem),
                              old_size,
                              old_size,
                              MREMAP_MAYMOVE | MREMAP_FIXED,
                              new_address + new_low_pages);
        if (remap_result == MAP_FAILED) {
            munmap_wrapper(new_address, new_size);
            return NULL;
        }
        return new_address +
               + (new_low_pages + in_page_offset(mem))
               - delta_low;
    }
}

char* eds_memmap_expand_low(char* mem, size_t size, size_t delta)
{
    assert((mem == NULL && size == 0) || (mem != NULL && size != 0));
    assert(size <= RSIZE_MAX);

    if (delta == 0) {
        return mem;
    }
    if (mem == NULL) {
        return eds_memmap_create(delta);
    }
    else if ((size + delta) < size || (size + delta) > RSIZE_MAX) {
        return NULL;
    }
    else if (size < mmap_treshold) {
        return expand_both_ends_small(mem, size, 0, delta);
    }
    else {
        return expand_both_ends_large(mem, size, 0, delta);
    }
}

char* eds_memmap_expand(char* mem, size_t size,
                        size_t delta_high, size_t delta_low)
{
    assert((mem == NULL && size == 0) || (mem != NULL && size != 0));
    assert(size <= RSIZE_MAX);

    if (mem == NULL) {
        return eds_memmap_create(delta_high + delta_low);
    }
    else if (delta_high == 0) {
        return eds_memmap_expand_low(mem, size, delta_low);
    }
    else if (delta_low == 0) {
        return eds_memmap_expand_high(mem, size, delta_high);
    }
    else if ((delta_low + delta_high < delta_low) ||
             (size + delta_low + delta_high < size) ||
             (size + delta_low + delta_high > RSIZE_MAX))
    {
        return NULL;
    }
    else if (size < mmap_treshold) {
        return expand_both_ends_small(mem, size, delta_high, delta_low);
    }
    else {
        return expand_both_ends_large(mem, size, delta_high, delta_low);
    }
}

static char*
shrink_high_small(char* mem, size_t size, size_t delta)
{
    if (delta == size) {
        free(mem);
        return NULL;
    }
    else {
        return realloc(mem, size - delta);
    }
}

static char*
shrink_large_to_small(char* mem, size_t size,
                      size_t delta_high, size_t delta_low)
{
    assert(size >= delta_high && size >= delta_low);
    assert(delta_high + delta_low >= delta_high);
    assert(size >= delta_high + delta_low);

    char *new_address;
    size_t new_size;

    new_size = size - delta_high - delta_low;
    new_address = malloc(new_size);
    if (new_address != NULL) {
        memcpy(new_address, mem + delta_low, new_size);
        munmap_wrapper(mem, total_size(mem, size));
    }
    return new_address;
}

static char*
shrink_high_large(char* mem, size_t size, size_t delta)
{
    assert(size >= delta);

    size_t scrap_pages;
    char *head;

    scrap_pages = total_size(mem, size) - total_size(mem, size - delta);
    if (scrap_pages != 0) {
        head = mem + total_size(mem, size) - scrap_pages;
        munmap_wrapper(head, scrap_pages);
    }
    return mem;
}

char* eds_memmap_shrink_high(char* mem, size_t size, size_t delta)
{
    assert((mem == NULL && size == 0) || (mem != NULL && size != 0));
    assert(size <= RSIZE_MAX);

    if (mem == NULL || delta > size) {
        return NULL;
    }
    else if (delta == 0) {
        return mem;
    }
    else if (delta == size) {
        eds_memmap_destroy(mem, size);
        return NULL;
    }
    else if (size < mmap_treshold) {
        return shrink_high_small(mem, size, delta);
    }
    else if (size - delta < mmap_treshold) {
        return shrink_large_to_small(mem, size, delta, 0);
    }
    else {
        return shrink_high_large(mem, size, delta);
    }
}

static char*
shrink_both_small(char* mem, size_t size,
                  size_t delta_high, size_t delta_low)
{
    assert(mem != NULL);
    assert(size > delta_low && size < mmap_treshold);
    assert(delta_low > 0);

    char* new_address;

    new_address = malloc(size - delta_low - delta_high);
    if (new_address == NULL) {
        return NULL;
    }
    memcpy(new_address, mem + delta_low, size - delta_high - delta_low);
    return new_address;
}

static char*
shrink_both_large(char* mem, size_t size,
                  size_t delta_high, size_t delta_low)
{
    assert(mem != NULL);
    assert(size >= mmap_treshold);
    assert(size > delta_low);
    assert(size > delta_high);
    assert(delta_low > 0);

    size_t scrap_low_pages;

    scrap_low_pages = page_boundary(mem + delta_low)
                      - page_boundary(mem);
    if (scrap_low_pages > 0) {
        munmap_wrapper(mem, scrap_low_pages);
    }
    mem += delta_low;
    size -= delta_low;
    return shrink_high_large(mem, size, delta_high);
}

char *eds_memmap_shrink_low(char* mem, size_t size, size_t delta)
{
    assert((mem == NULL && size == 0) || (mem != NULL && size != 0));
    assert(size <= RSIZE_MAX);

    if (delta == 0) {
        return mem;
    }
    else if (size == delta) {
        eds_memmap_destroy(mem, size);
        return NULL;
    }
    else if (delta > size) {
        return NULL;
    }
    else if (size < mmap_treshold) {
        return realloc(mem, size - delta);
    }
    else if (size - delta < mmap_treshold) {
        return shrink_large_to_small(mem, size, 0, delta);
    }
    else {
        return shrink_both_large(mem, size, 0, delta);
    }
}

char* eds_memmap_shrink(char* mem, size_t size,
                        size_t delta_high, size_t delta_low)
{
    assert((mem == NULL && size == 0) || (mem != NULL && size != 0));
    assert(size <= RSIZE_MAX);

    if (delta_low == 0) {
        return eds_memmap_shrink_high(mem, size, delta_high);
    }
    else if (mem == NULL || delta_high > size || delta_low > size) {
        return NULL;
    }
    else if (size < mmap_treshold) {
        return shrink_both_small(mem, size, delta_high, delta_low);
    }
    else if (size - delta_high - delta_low < mmap_treshold) {
        return shrink_large_to_small(mem, size, delta_high, delta_low);
    }
    else {
        return shrink_both_large(mem, size, delta_high, delta_low);
    }
}

