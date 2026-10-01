#include <cstddef>
#include <memory>
#include <numeric>
#include <utility>
#include <variant>

#include "DataBase.hpp"
#include "Allocator.hpp"
#include "DBTypes.hpp"
#include "Overload.hpp"
#include "State.hpp"
#include "utils.hpp"

namespace val_key {
    DataBase::DataBase() : state_(std::make_shared<State>(0, 0)), db_(DBType::allocator_type(state_)) {}

    bool DataBase::Contains(const Key& key) const {
        return db_.contains(key);
    }

    void DataBase::Erase(const Key& key) {
        db_.erase(key);
    }

    void DataBase::CheckKeyTtl(const Key& key) {
        if (db_.contains(key) && db_.at(key).ttl <= CurTime() && db_.at(key).ttl != -1) {
            db_.erase(key);
        }
    }

    void DataBase::Clear() {
        db_.clear();
    }

    size_t DataBase::Size() {
        return db_.size();
    }    

    auto DataBase::begin() -> decltype(std::declval<DBType>().begin()) {
        return db_.begin();
    }

    auto DataBase::end() -> decltype(std::declval<DBType>().begin()) {
        return db_.end();
    }

    size_t DataBase::UsageMem(const Key& key) {
        if (!Contains(key)) {
            return 0;
        }
        return key.size() + sizeof(db_.at(key)) 
            + std::visit(Overload {
                [](DBString& str) {
                    return str.size();
                },
                [](DBGeo& geo) {
                    return std::accumulate(geo.begin(), geo.end(), size_t(0), 
                        [](size_t acc, const auto& s) {
                            return s.first.size() + s.second->second.size() + sizeof(s.second->second);
                        }); 
                },
                [](auto& other) {
                    return std::accumulate(other.begin(), other.end(), size_t(0), 
                        [](size_t acc, const CountingString& s) {
                            return s.size() + acc;
                        });
                }
            }, db_.at(key).value);
    }

    auto DataBase::At(const Key& key) -> decltype(std::declval<DBType>().at(key)) {
        return db_.at(key);
    }

    auto DataBase::At(const Key& key) const -> decltype(std::declval<const DBType>().at(key)) {
        return db_.at(key);
    }

    void DataBase::SetMemLim(size_t bytes) {
        state_->limit_bytes = bytes;
    }

    size_t DataBase::GetMemLim() const {
        return state_->limit_bytes;
    }

    bool DataBase::IsEnoughMem() const {
        return state_->cur_mem_bytes < state_->limit_bytes || state_->limit_bytes == 0;
    }
} // namespace val_key
