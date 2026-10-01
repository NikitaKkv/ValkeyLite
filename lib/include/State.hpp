#pragma once

#include <cstddef>

namespace val_key {
    struct State {
        size_t limit_bytes;
        size_t cur_mem_bytes;
    };
} // namespace val_key

