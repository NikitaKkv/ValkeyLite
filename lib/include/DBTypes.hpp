#pragma once

#include <cstddef>
#include <memory>
#include <set>
#include <utility>
#include <variant>

#include "Allocator.hpp"
#include "State.hpp"

namespace val_key {
    using Key = CountingString;
    using DBString = CountingString;
    using DBList = CountingList<CountingString>;
    using DBSet = CountingHashSet<CountingString>;
    using TTLQueue = std::set<std::pair<int, Key>>;

    class DBGeo {
    public:
        using ByScore = CountingMultiMap<long long, DBString>;
        using ByMember = CountingHashMap<DBString, ByScore::iterator>;

        using scores_rev_iter = ByScore::reverse_iterator;

        DBGeo (std::shared_ptr<State> other);

        template<typename... Args>
        void Emplace(long long score, const DBString& memb) {
            members_[memb] = scores_.emplace(score, memb);
        }

        bool Contains(const DBString& key);

        bool Empty() const;

        auto At(const DBString& key) -> decltype(std::declval<ByMember>().at(key));

        auto begin() -> decltype(std::declval<ByMember>().begin());

        auto end() -> decltype(std::declval<ByMember>().end());

        auto ScoresBegin() -> decltype(std::declval<ByScore>().begin());

        auto ScoresEnd() -> decltype(std::declval<ByScore>().end());

        auto ScoresRBegin() -> decltype(std::declval<ByScore>().rbegin());

        auto ScoresREnd() -> decltype(std::declval<ByScore>().rend());

        auto LowerBound(long long score) -> decltype(std::declval<ByScore>().lower_bound(score));

        auto Find(const DBString& key) -> decltype(std::declval<ByMember>().find(key));

        size_t Size() const;

    private:
        ByScore scores_;
        ByMember members_;
    };

    struct ValueType {
        int ttl;
        std::variant<
            DBString, 
            DBList, 
            DBGeo, 
            DBSet
        > value;
    };
} // namespace val_key