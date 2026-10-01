#include "DBTypes.hpp"
#include "State.hpp"

#include <cstddef>
#include <memory>
#include <utility>

namespace val_key {
    DBGeo::DBGeo (std::shared_ptr<State> other) : scores_(other), members_(other) {}

    bool DBGeo::Contains(const DBString& key) {
        return members_.contains(key);
    }

    bool DBGeo::Empty() const {
        return members_.empty();
    }

    auto DBGeo::At(const DBString& key) -> decltype(std::declval<ByMember>().at(key)) {
        return members_.at(key);
    }

    auto DBGeo::begin() -> decltype(std::declval<ByMember>().begin()) { 
        return members_.begin(); 
    }

    auto DBGeo::end() -> decltype(std::declval<ByMember>().end()) { 
        return members_.end(); 
    }

    auto DBGeo::ScoresBegin() -> decltype(std::declval<ByScore>().begin()) { 
        return scores_.begin(); 
    }

    auto DBGeo::ScoresEnd() -> decltype(std::declval<ByScore>().end()) { 
        return scores_.end(); 
    }

    auto DBGeo::ScoresRBegin() -> decltype(std::declval<ByScore>().rbegin()) { 
        return scores_.rbegin(); 
    }

    auto DBGeo::ScoresREnd() -> decltype(std::declval<ByScore>().rend()) { 
        return scores_.rend(); 
    }

    auto DBGeo::LowerBound(long long score) -> decltype(std::declval<ByScore>().lower_bound(score)) { 
        return scores_.lower_bound(score); 
    }

    auto DBGeo::Find(const DBString& key) -> decltype(std::declval<ByMember>().find(key)) { 
        return members_.find(key); 
    }

    size_t DBGeo::Size() const { 
        return scores_.size(); 
    }
} // namespace val_key
