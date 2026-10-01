#include <chrono>

#include "utils.hpp"

namespace val_key {
    int CurTime() {
        auto duration = std::chrono::system_clock::now().time_since_epoch();
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
        return seconds;
    }
} // namespace val_key