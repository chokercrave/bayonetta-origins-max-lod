#pragma once

#include "common.hpp"

#define EXL_MODULE_NAME "origins-lod"

#define EXL_DEBUG
#define EXL_USE_FAKEHEAP

namespace exl::setting {
    /* How large the fake .bss heap will be. */
    constexpr size_t HeapSize = 0x5000;

    /* How large the JIT area will be for hooks. */
    constexpr size_t JitSize = 0x1000;

    /* How large the inline hook pool will be. */
    constexpr size_t InlinePoolSize = 0x1000;

    /* Formatting buffer size for logging. */
    constexpr size_t LogBufferSize = 512;

    /* Sanity checks. */
    static_assert(ALIGN_UP(JitSize, PAGE_SIZE) == JitSize, "");
    static_assert(ALIGN_UP(InlinePoolSize, PAGE_SIZE) == InlinePoolSize, "");
}
