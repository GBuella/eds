
#ifndef EDS_MEMMAP_STORAGE_H
#define EDS_MEMMAP_STORAGE_H

#include <cassert>
#include <cstddef>
#include <new>
#include <cstdint>

#include "eds_memmap.h"

namespace eds
{

template<typename char_type>
class mapped_storage
{
private:
    char_type* head;
    size_t length;

    void move_from(mapped_storage&& other)
    {
        head = other.head;
        length = other.length;
        other.head = nullptr;
        other.length = 0;
    }

public:

    typedef char_type value_type;
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;
    typedef char_type& reference;
    typedef const char_type& const_reference;
    typedef char_type* pointer;
    typedef const char_type* const_pointer;
    typedef char_type* iterator;
    typedef const char_type* const_iterator;
    typedef char_type* reverse_iterator;
    typedef const char_type* const_reverse_iterator;

    mapped_storage():
        head(nullptr),
        length(0)
    {}

    ~mapped_storage()
    {
        if (head != nullptr) {
            eds_memmap_destroy(head, length);
        }
    }

    explicit mapped_storage(size_type count):
        head(eds_memmap_create(count)),
        length(count)
    {
    }

    mapped_storage& operator=(mapped_storage&& other)
    {
        if (head != nullptr) {
            eds_memmap_destroy(head, length);
        }
        move_from(other);
        return *this;
    }

    explicit mapped_storage(mapped_storage&& other)
    {
        move_from(other);
    }

    mapped_storage(mapped_storage&) = delete;
    mapped_storage& operator=(mapped_storage&) = delete;

    size_type size() const noexcept
    {
        return length;
    }

    iterator begin() noexcept
    {
        return head;
    }

    iterator end() noexcept
    {
        return head + length;
    }

    const_iterator cbegin() const noexcept
    {
        return head;
    }

    const_iterator cend() const noexcept
    {
        return head + length;
    }

    reverse_iterator rbegin() noexcept
    {
        return end() - 1;
    }

    reverse_iterator rend() noexcept
    {
        return begin() - 1;
    }

    const_reverse_iterator crbegin() const noexcept
    {
        return cend() - 1;
    }

    const_reverse_iterator crend() const noexcept
    {
        return cbegin() - 1;
    }

private:

    void eds_size_delta_wrapper(char* (*eds_fun)(char*, size_t, size_t),
                                size_t count)
    {
        char* new_head;

        new_head = eds_fun(head, length, count);
        if (new_head == nullptr) {
            throw std::bad_alloc();
        }
        head = static_cast<char_type*>(new_head);
    }

public:

    void expand_high(size_type count)
    {
        eds_size_delta_wrapper(eds_memmap_expand_high, count);
        length += count;
    }

    void expand_low(size_type count)
    {
        eds_size_delta_wrapper(eds_memmap_expand_low, count);
        length += count;
    }

    void clear() noexcept
    {
        eds_memmap_destroy(head, length);
        head = nullptr;
        length = 0;
    }

    void shrink_high(size_type count)
    {
        if (length > count) {
            eds_size_delta_wrapper(eds_memmap_shrink_high, count);
            length -= count;
        }
        else if (length < count) {
            throw std::bad_alloc();
        }
        else {
            clear();
        }
    }

    void shrink_low(size_type count)
    {
        if (length > count) {
            eds_size_delta_wrapper(eds_memmap_shrink_low, count);
            length -= count;
        }
        else if (length < count) {
            throw std::bad_alloc();
        }
        else {
            clear();
        }
    }

    void shrink(size_t delta_high, size_t delta_low = 0)
    {
        if (delta_high == 0 and delta_low == 0) {
            return;
        }
        else if (head == nullptr
                or length < delta_high
                or length < delta_low
                or delta_high < delta_high + delta_low
                or length < delta_high + delta_low)
        {
            throw std::bad_alloc();
        }
        else if (length == delta_high + delta_low) {
            clear();
        }
        else {
            head = eds_memmap_shrink(head, length, delta_high, delta_low);
            length -= delta_high + delta_low;
        }
    }


    bool empty() const noexcept
    {
        return length == 0;
    }

    char_type* data() noexcept
    {
        return head;
    }

    const char_type* data() const noexcept
    {
        return head;
    }

    void swap(mapped_storage<char_type>& other)
    {
        std::swap(head, other.head);
        std::swap(length, other.length);
    }

    reference at(size_type pos)
    {
        if (pos >= length) {
            throw std::out_of_range();
        }
        return head[pos];
    }

    const_reference at(size_type pos) const
    {
        if (pos >= length) {
            throw std::out_of_range();
        }
        return head[pos];
    }

}; /* template mapped_storage */

} /* namespace eds */

#endif  /* EDS_MEMMAP_STORAGE_H */

