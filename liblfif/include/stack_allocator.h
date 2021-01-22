#pragma once

#include <cstdint>
#include <cstdlib>
#include <cassert>

#include <algorithm>
#include <iostream>

class StackAllocator {
  inline static uint8_t *m_base = nullptr;
  inline static uint8_t *m_head = nullptr;
  inline static uint8_t *m_end = nullptr;

  inline static uint8_t *m_max_head = nullptr;


  StackAllocator() = delete;

  static constexpr size_t alignment = 64;

public:
  static void init(size_t size) {
    m_base = static_cast<uint8_t *>(aligned_alloc(alignment, size));
    m_head = m_base;
    m_end  = m_base + size;
  }

  static void cleanup() {
    std::cerr << "Stack allocator peak memory usage: " << m_max_head - m_base << " bytes.\n";
    ::free(m_base);
  }

  static void *allocate(size_t size) {
    size_t bytes = (size + alignment - 1) & -alignment;
    assert(m_head + bytes < m_end);
    void *ptr = m_head;
    m_head += bytes;

    m_max_head = std::max(m_head, m_max_head);

    return ptr;
  }

  static void free(void *ptr) {
    m_head = static_cast<uint8_t *>(ptr);
  }
};
