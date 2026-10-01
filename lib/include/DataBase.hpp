#pragma once

#include "Allocator.hpp"
#include "DBTypes.hpp"
#include "State.hpp"

#include <cstddef>
#include <memory>
#include <unordered_map>
#include <utility>

namespace val_key {

    class DataBase {
    public:

        using DBType = CountingHashMap<Key, ValueType>;

        DataBase();

        bool Contains(const Key& key) const;

        template<typename... Args>
        void Emplace(Args&&... args) {
            db_.emplace(std::forward<Args>(args)...);
        }

        void Erase(const Key& key);

        void CheckKeyTtl(const Key& key);

        void Clear();

        size_t Size();

        auto begin() -> decltype(std::declval<DBType>().begin());

        auto end() -> decltype(std::declval<DBType>().end());

        size_t UsageMem(const Key& key);

        auto At(const Key& key) -> decltype(std::declval<DBType>().at(key));

        auto At(const Key& key) const -> decltype(std::declval<const DBType>().at(key));

        void SetMemLim(size_t bytes);

        size_t GetMemLim() const;

        bool IsEnoughMem() const;
        
        template<typename T>
        requires requires (std::shared_ptr<State> state) {
            T(state);
        }
        T Factory() {
            return T(state_);
        }

    private:
        std::shared_ptr<State> state_;
        DBType db_;
    }; 
} // namespace val_key



