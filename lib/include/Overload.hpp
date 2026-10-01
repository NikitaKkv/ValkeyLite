#pragma once

namespace val_key {
    template <typename... Ts>
    struct Overload : Ts... {
        using Ts::operator()...;
    };
} // namespace val_key

