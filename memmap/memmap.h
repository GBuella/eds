
#ifndef EDS_MEMMAP_H
#define EDS_MEMMAP_H

#include <cassert>
#include <cstddef>
#include <algorithm>
#include <memory>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <type_traits>

#include "mapped_storage.h"

namespace eds
{

template<typename type>
class memmap 
{
private:

    mapped_storage<char> storage;


    type* head;
    size_t length;

    const char* char_cbegin() const noexcept
    {
        return (char*)(void*)(head);
    }

    const char* char_cend() const noexcept
    {
        return (char*)(void*)(head + length);
    }

    template<typename... arg_types>
    static void create(type* address, arg_types&&... ctor_args)
    {
        if (address != nullptr) {
            ::new(address) type(ctor_args...);
        }
        else {
            /* Placement new must check for null pointer according to ISO,
               this allows the optimizer to get rid of that check.
               This can save a significant amount of time while bulk inserting
               many small elements e.g.: a million ints.
               It is not very portable, but the whole mremap thing is not
               portable anyways.
            */
            __builtin_unreachable();
        }
    }

public:

    memmap():
        head(nullptr),
        length(0)
    {
    }

    explicit memmap(const memmap& other):
        storage(other.length * sizeof(type)),
        head((type*)storage.begin()),
        length(other.length)
    {
        std::memcpy(begin(), other.cbegin(), size());
    }

    memmap& operator=(const memmap& other)
    {
        if (this != &other) {
            reserve_high(other.size());
            for (size_t index = length; index < length; ++index) {
                (head + index)->~type();
            }
            length = other.length;
            std::memcpy(begin(), other.cbegin(), size());
        }
        return *this;
    }

    typedef type value_type;
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;
    typedef type& reference;
    typedef const type& const_reference;
    typedef type* pointer;
    typedef const type* const_pointer;
    typedef type* iterator;
    typedef const type* const_iterator;
    typedef type* reverse_iterator;
    typedef const type* const_reverse_iterator;

    constexpr size_type max_size() const noexcept
    {
        return ((size_t(0) - 1) / 2) / sizeof(type);
    }

    size_type size() const noexcept
    {
        return length;
    }

    size_type capacity_high_raw() const noexcept
    {
        return (storage.cend() - char_cbegin());
    }

    size_t capacity_high() const noexcept
    {
        return (storage.cend() - char_cbegin()) / sizeof(type);
    }

    size_t capacity_low() const noexcept
    {
        return (char_cend() - storage.cbegin()) / sizeof(type);
    }

    size_t capacity() const noexcept
    {
        return capacity_high();
    }

    typename std::enable_if<std::is_trivially_move_constructible<type>::value>::type
    reserve_high(size_type count)
    {
        if (count > max_size()) {
            throw std::bad_alloc();
        }
        if (capacity_high() < count) {
            char* old_storage_begin = storage.begin();
            storage.expand_high((count - length) * sizeof(type));
            head = (type*)(storage.begin() + (char_cbegin() - old_storage_begin));
        }
    }

    void reserve_low(size_type count)
    {
        if (count > max_size()) {
            throw std::bad_alloc();
        }
        if (capacity_low() < count) {
            char* old_storage_begin = storage.begin();
            storage.expand_high((count - length) * sizeof(type));
            head = (type*)(storage.begin() + (char_cbegin() - old_storage_begin));
            head += count - length;
        }
    }

    void reserve(size_type new_cap)
    {
        reserve_high(new_cap);
    }

    void resize(size_type count)
    {
        for (size_t index = count; index < length; ++index) {
            (head + index)->~type();
        }
        reserve(count);
        for (size_t index = length; index < count; ++index) {
            create(head + index);
        }
        length = count;
    }

    void resize(size_type count, const value_type& value)
    {
        for (size_t index = count; index < length; ++index) {
            (head + index)->~type();
        }
        reserve(count);
        for (size_t index = length; index < count; ++index) {
            create(head + index, value);
        }
        length = count;
    }

    bool empty() const noexcept
    {
        return length == 0;
    }

    iterator begin() noexcept
    {
        return head;
    }

    iterator end() noexcept
    {
        return begin() + size();
    }

    const_iterator cbegin() const noexcept
    {
        return head;
    }

    const_iterator cend() const noexcept
    {
        return cbegin() + size();
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

    reference operator[](size_type position) noexcept
    {
        return head[position];
    }

    const_reference operator[](size_type position) const noexcept
    {
        return head[position];
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

    reference front() noexcept
    {
        return *head;
    }

    const_reference front() const noexcept
    {
        return *head;
    }

    reference back() noexcept
    {
        return head[length - 1];
    }

    const_reference back() const noexcept
    {
        return head[length - 1];
    }

private:

    void reserve_for_push(bool at_high)
    {
        size_t new_size;

        if (at_high and capacity_high() != size()) {
            return;
        }
        if (not at_high and capacity_low() != size()) {
            return;
        }
        new_size = size() * 2;
        if (new_size < 1) {
            new_size = 1;
        }
        else if (new_size > max_size()) {
            new_size = max_size();
            if (new_size == size()) {
                throw std::bad_alloc();
            }
        }
        if (at_high) {
            reserve_high(new_size);
        }
        else {
            reserve_low(new_size);
        }
    }

public:

    template<typename... arg_types>
    void emplace_back(arg_types&&... ctor_args)
    {
        reserve_for_push(true);
        create(head + length, ctor_args...);
        ++length;
    }

    template<typename... arg_types>
    void emplace_front(arg_types&&... ctor_args)
    {
        reserve_for_push(false);
        --head;
        create(head, ctor_args...);
        ++length;
    }

    void push_back(const type& value)
    {
        emplace_back(value);
    }

    void push_front(const type& value)
    {
        emplace_front(value);
    }

    void push_back(type&& value)
    {
        emplace_back(value);
    }

    void push_front(type&& value)
    {
        emplace_front(value);
    }

    void pop_back()
    {
        --length;
        head[length].~type();
    }

    void pop_front()
    {
        --length;
        head->~type();
    }

    void clear() noexcept
    {
        for (auto item : *this) {
            item->~type();
        }
        length = 0;
    }

    pointer data() noexcept
    {
        return head;
    }

    const_pointer data() const noexcept
    {
        return head;
    }

    void swap(memmap<type>& other)
    {
        std::swap(storage, other.storage);
        std::swap(head, other.head);
        std::swap(length, other.length);
    }

    void shrink_to_fit()
    {
        storage.shrink(char_cend() - storage.cend(),
                       char_cbegin() - storage.begin());
    }

    void assign(size_type count, const type& value )
    {
        clear();
        resize(count, value);
    }

    template<class input_iterator>
    void assign(input_iterator first, input_iterator last)
    {
        pointer item;
        input_iterator input;

        clear();
        input = first;
        item = head;
        reserve(last - first);
        while (input != last) {
            const type& input_item = *input;
            create(item, input_item);
        }
    }

}; /* template memmap */

template<typename type>
bool operator==(const memmap<type>& x, const memmap<type>& y)
{
    if (x.size() != y.size()) {
        return false;
    }
    auto xi = x.cbegin();
    auto yi = y.cbegin();
    while (xi != x.cend()) {
        if (*xi != *yi) {
            return false;
        }
        ++xi;
        ++yi;
    }
    return true;
}

template<typename type>
bool operator!=(const memmap<type>& x, const memmap<type>& y)
{
    return not (x == y);
}


} /* namespace eds */

#endif /* EDS_MEMMAP_H */
