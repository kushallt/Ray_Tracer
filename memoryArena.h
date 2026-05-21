#pragma once
#include <cstddef>
#include <memory>
#include <new>

class MemoryArena {
private:
    std::byte *buffer;
    size_t offset;
    size_t capacity;
public:
    MemoryArena(size_t size) : buffer(static_cast<std::byte*>(::operator new(size))), offset(0), capacity(size) {}
    ~MemoryArena(){
        ::operator delete(buffer);
    }
    void* allocate(std::size_t size, std::size_t alignment = alignof(std::max_align_t)){
        std::byte *current = buffer + offset;
        size_t space = capacity - offset;
        void* aligned_ptr = current;
        if (std::align(alignment, size, aligned_ptr, space) == nullptr) {
            throw std::bad_alloc();
        }
        offset = (std::byte *)aligned_ptr - buffer + size;
        return aligned_ptr;
    }
    void reset(){
        offset = 0;
    }
};