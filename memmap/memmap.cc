

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif


#include <unistd.h>
#include <climits>
#include <sys/mman.h>

#include <new>

#include "memmap.h"

static size_t page_size = 1;

static void check_pagesize()
{
    if (page_size <= 1) {
        size_t size = sysconf(_SC_PAGESIZE);
        if (size > 1) {
            page_size = size;
        }
        else {
            throw std::bad_alloc();
        }
    }
}

static eds::raw_memmap::page_size = 1;

eds::raw_memmap::raw_memmap():
    raw_pointer(nullptr),
    raw_size(0),
    head(nullptr)
{
}

eds::raw_memmap& eds::raw_memmap::operator=(raw_memmap&&)
{
    if (raw_pointer != nullptr) {
        munmap(raw_pointer, raw_size);
    }

    raw_pointer = other.raw_pointer;
    raw_size = other.raw_size;
    head = other.head;
    length = other.length;

    other.raw_pointer = nullptr;
    other.raw_size = 0;
    other.head = nullptr;
    other.length = nullptr;

    return *this;
}

eds::raw_memmap::raw_memmap(raw_memmap&& other)
{
    raw_pointer = nullptr;

    *this = other;
}


eds::raw_memmap::~raw_memmap()
{
    if (raw_pointer != nullptr) {
        munmap(raw_pointer, raw_size);
    }
}

static size_t round_up(size_t value)
{
    if (value % page_size == 0) {
        return value;
    }
    else if ((value / page_size) == SIZE_MAX / page_size) {
        throw std::bad_alloc();
    }
    else {
        return ((value / page_size) + 1) * page_size;
    }
}

void eds::raw_memmap::append_at_high_address(size_t delta,
                                             unsigned growth_factor)
{
    if ((length + delta) < length) {             //    check for overflow
        throw std::bad_alloc();
    }
    else if (capacity_high() >= (length + delta)) {
        length += delta;    // raw_memmap already has enough capacity,
        return;             // this includes an empty raw_memmap when delta is 0
    }
    else if (raw_pointer == nullptr) {
        void* new_pointer;
        size_t new_size;

        if (delta < mmap_treshold) {
            new_size = delta;
            new_pointer = new char[new_size];
        }
        else {
            check_pagesize();
            new_size = round_up(delta);
            new_pointer = mmap(nullptr, new_size,
                    PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            if (new_pointer == MAP_FAILED) {
                throw std::bad_alloc();
            }
        }

            /*  At this point no member has changed
                and no memory is allocated, thus
                strong exception guarantee is kept */

        raw_pointer = new_pointer;
        raw_size = new_size;
        head = raw_pointer;
        length = delta;
    }
    else {
        void* new_pointer;
        size_t new_size;

        if ((head - raw_pointer) + length > (SIZE_MAX - delta)) {
            throw std::bad_alloc();
        }
        check_pagesize();
        if (raw_size + delta < mmap_treshold) {
            new_size = raw_size + delta;
            new_pointer = new char[new_size];
            std::memcpy(new_pointer + (head - raw_pointer), head, length);
            delete[] raw_pointer;
        }
        else if (raw_size < mmap_treshold) {
            new_size = round_up((head - raw_pointer) + length + delta);
            new_pointer = mmap(nullptr, new_size,
                               PROT_READ | PROT_WRITE,
                               MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            if (new_pointer == MAP_FAILED) {
                throw std::bad_alloc();
            }
            std::memcpy(new_pointer + (head - raw_pointer), head, length);
            delete[] raw_pointer;
        }
        else {
            new_size = round_up((head - raw_pointer) + length + delta);
            new_pointer = mremap(raw_pointer, raw_size, new_size, MREMAP_MAYMOVE);
            if (new_pointer == MAP_FAILED) {
                throw std::bad_alloc();
            }
        }
        head = new_pointer + (head - raw_pointer);
        raw_pointer = new_pointer;
        raw_size = new_size;
        length += delta;
    }
}

void eds::raw_memmap::append_at_low_address(size_t delta, unsigned growth_factor)
{
  void* new_pointer;
  size_t new_page_count;

  if (delta == 0) {
    return;
  }
  else if ((length + delta) < length) {
    throw std::bad_alloc();
  }
  else if (raw_pointer == nullptr) {
    append_at_high_address(delta);
  }
  else if (capacity_low() >= delta) {
    head -= delta;
    length += delta;
    return;
  }
  else {
    void* new_pointer;
    void* relocated;
    size_t size_increase;

    check_pagesize();
    size_increase = round_up(delta - (head - raw_pointer));
    new_pointer = mmap(nullptr,
                       raw_size + size_increase,
                       PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (new_pointer == MAP_FAILED) {
      throw std::bad_alloc();
    }
    relocated == mremap(raw_pointer, raw_size, raw_size,
                            MREMAP_MAYMOVE | MREMAP_FIXED,
                            new_pointer + size_increase);
    if (relocated == MAP_FAILED) {          // Should not happend
      munmap(new_pointer, raw_size + size_increase);
      throw std::bad_alloc();
    }
    head = (relocated + (head - raw_pointer)) - delta;
    raw_pointer = new_pointer;
    raw_size + size_increase;
    length += delta;
  }
}

void eds::raw_memmap::swap(raw_memmap& other)
{
  swap(raw_pointer, other.raw_pointer);
  swap(raw_size, other.raw_size);
  swap(head, other.head);
  swap(length, other.length);
}

