#include <algorithm>
#include <cctype>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <expected>
#include <format>
#include <map>
#include <numbers>
#include <optional>
#include <regex>
#include <string>
#include <system_error>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

#include "ValKeyServer.hpp"
#include "DBTypes.hpp"
#include "DataBase.hpp"
#include "Overload.hpp"
#include "utils.hpp"

namespace val_key {
    Error::Error(const std::string& err) : err_(err) {}

    std::string Error::What() {
        return err_;
    }

    char ToLower(unsigned char c) {
        if (c >= 'A' && c <= 'Z') return c + 32;
        return c;
    }

    const std::map<std::string, double> ValKeyServer::kUnits = {
        {"km", 1000.0},
        {"m", 1.0},
        {"mi", 1609.34},
        {"ft", 0.3048}
    };


    ValKeyServer::ValKeyServer() : 
            commands_ {
                {"set", {Set, 2, 2, true}},
                {"get", {Get, 1, 1, false}},
                {"strlen", {Strlen, 1, 1, false}},
                {"append", {Append, 2, 2, true}},

                {"lpush", {LPush, 2, -1, true}},
                {"rpush", {RPush, 2, -1, true}},
                {"lpop", {LPop, 1, 2, false}},
                {"rpop", {RPop, 1, 2, false}},
                {"llen", {LLen, 1, 1, false}},
                {"lrange", {LRange, 3, 3, false}},
                {"lindex", {LIndex, 2, 2, false}},
                {"lset", {LSet, 3, 3, true}},
                {"linsert", {LInsert, 4, 4, true}},

                {"sadd", {SAdd, 2, -1, true}},
                {"srem", {SRem, 2, -1, false}},
                {"sismember", {SIsMember, 2, 2, false}},
                {"smembers", {SMembers, 1, 1, false}},
                {"scard", {SCard, 1, 1, false}},
                {"sunion", {SUnion, 1, -1, false}},
                {"sinter", {SInter, 1, -1, false}},
                {"sdiff", {SDiff, 1, -1, false}},
                {"smove", {SMove, 3, 3, false}},

                {"geoadd", {GeoAdd, 4, -1, true}},
                {"geopos", {GeoPos, 2, -1, false}},
                {"geodist", {GeoDist, 3, 4, false}},
                {"geosearch", {GeoSearch, 8, 10, false}},
                {"geosearchstore", {GeoSearchStore, 9, 11, false}},

                {"ttl", {TTL, 1, 1, false}},
                {"type", {Type, 1, 1, false}},
                {"del", {Del, 1, -1, false}},
                {"exists", {Exists, 1, -1, false}},
                {"keys", {Keys, 1, 1, false}},
                {"flushdb", {FlushDB, 0, 0, false}},
                {"dbsize", {DBsize, 0, 0, false}},  
            },
            subcommands_ {
                {
                    "config", {
                        {"set", {ConfigSet, 2, 2, false}},
                        {"get", {ConfigGet, 1, 1, false}}
                    }
            
                },
                {
                    "memory", {
                        {"usage", {MemoryUsage, 1, 1, false}},
                    }
            
                },
            },
            db_(), q_() {}

    ValKeyServer::RetType ValKeyServer::Query(std::vector<std::string>& args) {
        size_t len = args.size();
        const std::string& cmd = args[0];

        ClearDB(db_, q_);

        auto process = [len, this, &args](auto& obj, auto& cmd, int miss) -> RetType {
            if (obj.is_increasing) {
                if (!db_.IsEnoughMem()) {
                    return Error("OOM command not allowed when used memory > " + std::to_string(db_.GetMemLim()));
                }
            }

            if ((len - miss > obj.max_arg_cnt && obj.max_arg_cnt != -1) || len - miss < obj.min_arg_cnt) {
                return Error("ERR wrong number of arguments for " + cmd + " cmd");
            }
            
            std::vector<DBString> args_db;
            for (size_t i = miss; i < args.size(); ++i) {
                args_db.emplace_back(db_.Factory<DBString>() + args[i].c_str());
            }

            return obj.com(db_, args_db);
        };

        if (cmd == "expire") {
            if (len - 1 != 2) {
                return Error("ERR wrong number of arguments for expire cmd");
            }

            std::vector<DBString> args_db;
            for (size_t i = 1; i < args.size(); ++i) {
                args_db.emplace_back(db_.Factory<DBString>() + args[i].c_str());
            }

            return Expire(q_, db_, args_db);
        }
        else if (commands_.contains(cmd)) {
            auto& cmd_obj = commands_[cmd];
            
            return process(cmd_obj, cmd, 1);
        }
        else if (len >= 2 && subcommands_.contains(cmd) && subcommands_[cmd].contains(args[1])) {
            std::ranges::transform(args[1], args[1].begin(), ToLower);
            const std::string& subcmd = args[1];

            auto& subcmd_obj = subcommands_[cmd][subcmd];

            return process(subcmd_obj, subcmd, 2);
        }
        else {
            return Error("-ERR unknown cmd" + cmd + ", with args beginning with: ...");
        }
    }


    //----------------------- Special -------------------------
    void ValKeyServer::ClearDB(DataBase& db, TTLQueue& q) {
        while (!q.empty() && q.begin()->first <= CurTime()) {
            db.Erase(q.begin()->second);
            q.erase(q.begin());
        }
    }

    double ValKeyServer::GetDist(double lon1, double lat1, double lon2, double lat2, const std::string& unit) {
        const double r = 6372800.0;
        
        const double dlat = (lat2 - lat1) * std::numbers::pi / 180.0;
        const double dlon = (lon2 - lon1) * std::numbers::pi / 180.0;

        const double a = std::sin(dlat / 2) * std::sin(dlat / 2) +
                        std::cos(lat1 * std::numbers::pi / 180.0) * std::cos(lat2 * std::numbers::pi / 180.0) *
                            std::sin(dlon / 2) * std::sin(dlon / 2);
        
        const double c = 2 * std::asin(std::sqrt(a));
        
        // Получаем расстояние в метрах
        double dist_meters = r * c;

        return dist_meters / kUnits.at(unit);
    }

    uint64_t Interleave64(uint32_t xlo, uint32_t ylo) {
        static const uint64_t kB[] = {0x5555555555555555ULL, 0x3333333333333333ULL, 0x0F0F0F0F0F0F0F0FULL, 0x00FF00FF00FF00FFULL, 0x0000FFFF0000FFFFULL};
        static const unsigned int kS[] = {1, 2, 4, 8, 16};
        uint64_t x = xlo;
        uint64_t y = ylo;
        x = (x | (x << kS[4])) & kB[4];
        y = (y | (y << kS[4])) & kB[4];
        x = (x | (x << kS[3])) & kB[3];
        y = (y | (y << kS[3])) & kB[3];
        x = (x | (x << kS[2])) & kB[2];
        y = (y | (y << kS[2])) & kB[2];
        x = (x | (x << kS[1])) & kB[1];
        y = (y | (y << kS[1])) & kB[1];
        x = (x | (x << kS[0])) & kB[0];
        y = (y | (y << kS[0])) & kB[0];
        return x | (y << 1); 
    }

    uint64_t Deinterleave64(uint64_t interleaved) {
        static const uint64_t kB[] = {0x5555555555555555ULL, 0x3333333333333333ULL, 0x0F0F0F0F0F0F0F0FULL, 0x00FF00FF00FF00FFULL, 0x0000FFFF0000FFFFULL, 0x00000000FFFFFFFFULL};
        static const unsigned int kS[] = {0, 1, 2, 4, 8, 16};
        uint64_t x = interleaved;
        uint64_t y = interleaved >> 1;
        x = (x | (x >> kS[0])) & kB[0];
        y = (y | (y >> kS[0])) & kB[0];
        x = (x | (x >> kS[1])) & kB[1];
        y = (y | (y >> kS[1])) & kB[1];
        x = (x | (x >> kS[2])) & kB[2];
        y = (y | (y >> kS[2])) & kB[2];
        x = (x | (x >> kS[3])) & kB[3];
        y = (y | (y >> kS[3])) & kB[3];
        x = (x | (x >> kS[4])) & kB[4];
        y = (y | (y >> kS[4])) & kB[4];
        x = (x | (x >> kS[5])) & kB[5];
        y = (y | (y >> kS[5])) & kB[5];
        return x | (y << 32); 
    }

    long long ValKeyServer::GetScore(double lon, double lat) {
        const double lat_min = -85.05112878;
        const double lat_max = 85.05112878;
        const double lon_min = -180.0;
        const double lon_max = 180.0;
        
        double lat_offset = (lat - lat_min) / (lat_max - lat_min);
        double lon_offset = (lon - lon_min) / (lon_max - lon_min);
        
        uint64_t lat_val_64 = static_cast<uint64_t>(lat_offset * 4294967296.0);
        uint64_t lon_val_64 = static_cast<uint64_t>(lon_offset * 4294967296.0);
        
        if (lat_val_64 >= 4294967296ULL) lat_val_64 = 4294967295ULL;
        if (lon_val_64 >= 4294967296ULL) lon_val_64 = 4294967295ULL;
        
        uint32_t lat_val = static_cast<uint32_t>(lat_val_64);
        uint32_t lon_val = static_cast<uint32_t>(lon_val_64);
        
        return static_cast<long long>(Interleave64(lat_val, lon_val));
    }

    std::pair<double, double> ValKeyServer::GetLonlat(long long score) {
        const double k_lat_min = -85.05112878;
        const double k_lat_max = 85.05112878;
        const double k_lon_min = -180.0;
        const double k_lon_max = 180.0;
        
        uint64_t hash_sep = Deinterleave64(static_cast<uint64_t>(score));
        uint32_t lat_val = static_cast<uint32_t>(hash_sep);
        uint32_t lon_val = static_cast<uint32_t>(hash_sep >> 32);
        
        double lat_scale = k_lat_max - k_lat_min;
        double lon_scale = k_lon_max - k_lon_min;
        
        double lat_min = k_lat_min + (lat_val * 1.0 / 4294967296.0) * lat_scale;
        double lat_max = k_lat_min + ((lat_val + 1) * 1.0 / 4294967296.0) * lat_scale;
        
        double lon_min = k_lon_min + (lon_val * 1.0 / 4294967296.0) * lon_scale;
        double lon_max = k_lon_min + ((lon_val + 1) * 1.0 / 4294967296.0) * lon_scale;
        
        double lat = (lat_min + lat_max) / 2.0;
        double lon = (lon_min + lon_max) / 2.0;
        
        return {lon, lat};
    }

    bool ValKeyServer::Cmp(const DBString& s1, const std::string& s2) {
        if (s1.size() != s2.size()) {
            return false;
        }
        for (size_t i = 0; i < s1.size(); ++i) {
            if (ToLower(s1[i]) != ToLower(s2[i])) {
                return false;
            }
        }
        return true;
    }

    auto ValKeyServer::GeoSearchParse(const std::vector<DBString>& args, size_t start)
        -> std::expected<GeoSearchParseResult, Error> {

        GeoSearchParseResult pr;

        for (size_t i = start; i < args.size(); ++i) {
            if (Cmp(args[i], "fromlonlat")) {
                if (pr.lon || i + 2 >= args.size()) {
                    return std::unexpected<Error>("ERR syntax error");
                }
                pr.lon = IsCorrectInput<double>(args[i + 1]);
                pr.lat = IsCorrectInput<double>(args[i + 2]);
                if (!pr.lon || !pr.lat) {
                    return std::unexpected<Error>("ERR value is not a valid float");
                }
                if (std::abs(*pr.lon) > 180 || std::abs(*pr.lat) > 85.05112878) {
                    return std::unexpected<Error>("ERR invalid longitude,latitude pair");
                }
                i += 2;
            }
            else if (Cmp(args[i], "byradius")) {
                if (pr.rad || i + 2 >= args.size()) {
                    return std::unexpected<Error>("ERR syntax error");
                }
                if (!kUnits.contains(std::string(args[i + 2]))) {
                    return std::unexpected<Error>("ERR unsupported unit provided. any zip code or distance must be a measure distance");;
                }
                pr.unit = args[i + 2];
                pr.rad = IsCorrectInput<double>(args[i + 1]);
                if (!pr.rad) {
                    return std::unexpected<Error>("ERR value is not an integer or out of range");
                }
                i += 2;
            }
            else if (Cmp(args[i], "count")) {
                if (pr.cnt || i + 1 >= args.size()) {
                    return std::unexpected<Error>("ERR syntax error");
                }
                pr.cnt = IsCorrectInput<size_t>(args[i + 1]);
                if (!pr.cnt) {
                    return std::unexpected<Error>("ERR value is not an integer or out of range");
                }
                i += 1;
            }
            else if (Cmp(args[i], "asc")) {
                if (pr.sort) {
                    return std::unexpected<Error>("ERR syntax error");
                }  
                pr.sort = "asc";
            }
            else if (Cmp(args[i], "desc")) {
                if (pr.sort) {
                    return std::unexpected<Error>("ERR syntax error");
                }  
                pr.sort = "desc";
            }
            else {
                return std::unexpected<Error>("ERR syntax error");
            }
        }

        if (!pr.lon || !pr.rad) {
            return std::unexpected<Error>("ERR syntax error");
        }

        if (!pr.sort) {
            pr.sort = "asc";
        }

        return pr;
    }

    //----------------------- STRING -------------------------
    ValKeyServer::RetType ValKeyServer::Set(DataBase& db, std::vector<DBString>& args) {

        const Key& key = args[0];
        db.CheckKeyTtl(key);

        const DBString& value = args[1];

        if (!db.Contains(key)) {
            db.Emplace(key, ValueType{-1, value});
            return std::string("OK");
        }
        else if (auto* s = std::get_if<DBString>(&db.At(key).value)) {
            db.At(key).ttl = -1;
            *s = value;
            return std::string("OK");
        }
        else {
            return Error("WRONGTYPE Operation against a key holding the wrong kind of value");
        }
    }

    ValKeyServer::RetType ValKeyServer::Get(DataBase& db, std::vector<DBString>& args) {
        const Key& key = args[0];
        db.CheckKeyTtl(key);
        if (!db.Contains(key)) {
            return Nil();
        }
        else if (auto* s = std::get_if<DBString>(&db.At(key).value)) {
            return BulkString{std::string(*s)};
        }
        else {
            return Error("WRONGTYPE Operation against a key holding the wrong kind of value");
        }
    }

    ValKeyServer::RetType ValKeyServer::Strlen(DataBase& db, std::vector<DBString>& args) {
        const Key& key = args[0];
        db.CheckKeyTtl(key);

        if (!db.Contains(key)) {
            return 0;
        }
        else if (auto* s = std::get_if<DBString>(&db.At(key).value)) {
            return static_cast<long long>(s->size());
        }
        else {
            return Error("WRONGTYPE Operation against a key holding the wrong kind of value");
        }
    }

    ValKeyServer::RetType ValKeyServer::Append(DataBase& db, std::vector<DBString>& args) {

        const Key& key = args[0];
        db.CheckKeyTtl(key);
        const DBString& value = args[1];

        if (!db.Contains(key)) {
            db.Emplace(key, ValueType{-1, db.Factory<DBString>()});
        }

        if (auto* s = std::get_if<DBString>(&db.At(key).value)) {
            *s += value;
            return static_cast<long long>(s->size());
        }
        else {
            return Error("WRONGTYPE Operation against a key holding the wrong kind of value");
        }
    }

    //----------------------- LIST -------------------------

    ValKeyServer::RetType ValKeyServer::LPush(DataBase& db, std::vector<DBString>& args) {
        const Key& key = args[0];
        db.CheckKeyTtl(key);

        if (!db.Contains(key)) {
            db.Emplace(key, ValueType{-1, db.Factory<DBList>()});
        }

        if (auto* l = std::get_if<DBList>(&db.At(key).value)) {
            for (size_t i = 1; i < args.size(); ++i) {
                l->push_front(args[i]);
            }
            return static_cast<long long>(l->size());
        }
        else {
            return Error("WRONGTYPE Operation against a key holding the wrong kind of value");
        }
    } 

    ValKeyServer::RetType ValKeyServer::RPush(DataBase& db, std::vector<DBString>& args) {
        const Key& key = args[0];
        db.CheckKeyTtl(key);

        if (!db.Contains(key)) {
            db.Emplace(key, ValueType{-1, db.Factory<DBList>()});
        }

        if (auto* l = std::get_if<DBList>(&db.At(key).value)) {
            for (size_t i = 1; i < args.size(); ++i) {
                l->push_back(args[i]);
            }
            return static_cast<long long>(l->size());
        }
        else {
            return Error("WRONGTYPE Operation against a key holding the wrong kind of value");
        }
    } 

    ValKeyServer::RetType ValKeyServer::LPop(DataBase& db, std::vector<DBString>& args) {
        const Key& key = args[0];
        db.CheckKeyTtl(key);

        size_t cnt = -1;
        if (args.size() == 2) {
            auto new_cnt = IsCorrectInput<size_t>(args[1]);
            if (!new_cnt) {
                return Error("ERR value is not an integer or out of range");
            }
            cnt = *new_cnt;
        }

        // По стандарту Redis при отсутствии ключа должен возвращаться Nil.
        if (!db.Contains(key)) {
            if (cnt != -1) {
                return RESPArray{}; 
            } 
            else {
                return Nil{};       
            }
        }
        else if (auto* l = std::get_if<DBList>(&db.At(key).value)) {

            if (cnt == -1) {
                std::string result(l->front());
                l->pop_front();
                if (l->empty()) {
                    db.Erase(key);
                }
                return result;
            }

            RESPArray values;
            cnt = std::min(cnt, l->size());

            for (size_t i = 0; i < cnt; ++i) {
                values.emplace_back(BulkString{std::string(l->front())}); 
                l->pop_front();
            }

            if (l->empty()) {
                db.Erase(key);
            }

            return values;
        }
        else {
            return Error("WRONGTYPE Operation against a key holding the wrong kind of value");
        }
    }

    ValKeyServer::RetType ValKeyServer::RPop(DataBase& db, std::vector<DBString>& args) {
        const Key& key = args[0];
        db.CheckKeyTtl(key);

        size_t cnt = -1;
        if (args.size() == 2) {
            auto new_cnt = IsCorrectInput<size_t>(args[1]);
            if (!new_cnt) {
                return Error("ERR value is not an integer or out of range");
            }
            cnt = *new_cnt;
        }

        // По стандарту Redis при отсутствии ключа должен возвращаться Nil.
        if (!db.Contains(key)) {
            if (cnt != -1) {
                return RESPArray{}; 
            } 
            else {
                return Nil{};       
            }
        }
        else if (auto* l = std::get_if<DBList>(&db.At(key).value)) {

            if (cnt == -1) {
                std::string result(l->back());
                l->pop_back();
                if (l->empty()) {
                    db.Erase(key);
                }
                return result;
            }

            RESPArray values;
            cnt = std::min(cnt, l->size());

            for (size_t i = 0; i < cnt; ++i) {
                values.emplace_back(BulkString{std::string(l->back())}); 
                l->pop_back();
            }

            if (l->empty()) {
                db.Erase(key);
            }

            return values;
        }
        else {
            return Error("WRONGTYPE Operation against a key holding the wrong kind of value");
        }
    }

    ValKeyServer::RetType ValKeyServer::LLen(DataBase& db, std::vector<DBString>& args) {
        const Key& key = args[0];
        db.CheckKeyTtl(key);

        if (!db.Contains(key)) {
            return 0ll;
        }
        else if (auto* l = std::get_if<DBList>(&db.At(key).value)) {
            return static_cast<long long>(l->size());
        }
        else {
            return Error("WRONGTYPE Operation against a key holding the wrong kind of value");
        }
    }


    ValKeyServer::RetType ValKeyServer::LRange(DataBase& db, std::vector<DBString>& args) {
        const Key& key = args[0];
        db.CheckKeyTtl(key);

        auto start = IsCorrectInput<int>(args[1]);
        auto stop = IsCorrectInput<int>(args[2]);

        if (!start || !stop) {
            return Error("ERR value is not an integer or out of range");
        }

        RESPArray result;

        if (!db.Contains(key)) {
            return result;
        }
        else if (auto* l = std::get_if<DBList>(&db.At(key).value)) {
            if (*start < 0) {
                *start = std::max(*start + l->size(), size_t(0));
            }
            if (*stop < 0) {
                *stop = std::max(*stop + l->size(), size_t(0));
            }
            *start = std::min(*start, int(l->size()) - 1);
            *stop = std::min(*stop, int(l->size()) - 1);

            auto it = l->begin();
            for (size_t i = 0; i < *start; ++i) {
                ++it;
            }
            for (int i = *start; i <= *stop; ++i) {
                result.emplace_back(BulkString{std::string(*it)});
                ++it;
            }
            return result;
        }
        else {
            return Error("WRONGTYPE Operation against a key holding the wrong kind of value");
        }
    }

    ValKeyServer::RetType ValKeyServer::LIndex(DataBase& db, std::vector<DBString>& args) {
        const Key& key = args[0];
        db.CheckKeyTtl(key);

        auto idx_opt = IsCorrectInput<int>(args[1]);
        
        if (!idx_opt) {
            return Error("ERR value is not an integer or out of range");
        }

        int idx = *idx_opt;

        if (!db.Contains(key)) {
            return Nil();
        }
        else if (auto* l = std::get_if<DBList>(&db.At(key).value)) {
            if (*idx_opt < 0) *idx_opt += l->size();
            if (*idx_opt < 0 || *idx_opt >= l->size()) {
                return Nil{};
            }
            auto it = l->begin();
            for (size_t i = 0; i < *idx_opt; ++i) {
                ++it;
            }
            return BulkString{std::string(*it)};
        }
        else {
            return Error("WRONGTYPE Operation against a key holding the wrong kind of value");
        }
    }

    ValKeyServer::RetType ValKeyServer::LSet(DataBase& db, std::vector<DBString>& args) {
        const Key& key = args[0];
        db.CheckKeyTtl(key);

        auto idx_opt = IsCorrectInput<int>(args[1]);
        
        if (!idx_opt) {
            return Error("ERR value is not an integer or out of range");
        }

        int idx = *idx_opt;

        if (!db.Contains(key)) {
            return Error("ERR no such key");
        }
        else if (auto* l = std::get_if<DBList>(&db.At(key).value)) {
            if (*idx_opt < 0) *idx_opt += l->size();
            if (*idx_opt < 0 || *idx_opt >= l->size()) {
                return Error("ERR index out of range");
            }
            auto it = l->begin();
            for (size_t i = 0; i < *idx_opt; ++i) {
                ++it;
            }
            *it = args[2];
            return std::string("OK");
        }
        else {
            return Error("WRONGTYPE Operation against a key holding the wrong kind of value");
        }
    }

    ValKeyServer::RetType ValKeyServer::LInsert(DataBase& db, std::vector<DBString>& args) {
        const Key& key = args[0];
        db.CheckKeyTtl(key);

        std::ranges::transform(args[1], args[1].begin(), ToLower);
        DBString& where = args[1];

        bool is_before = false;

        if (where == "before") {
            is_before = true;
        }
        else if (where != "before" && where != "after") {
            return Error("ERR syntax error");
        }

        if (!db.Contains(key)) {
            return 0ll;
        }
        else if (auto* l = std::get_if<DBList>(&db.At(key).value)) {
            auto it = std::ranges::find(*l, args[2]);
            if (it == l->end()) {
                return -1ll;
            }
            else {
                is_before ? l->insert(it, args[3]) : l->insert(++it, args[3]);
                return static_cast<long long>(l->size());
            }
        }
        else {
            return Error("WRONGTYPE Operation against a key holding the wrong kind of value");
        }
    }

    //----------------------- SET -------------------------

    ValKeyServer::RetType ValKeyServer::SAdd(DataBase& db, std::vector<DBString>& args) {
        const Key& key = args[0];
        db.CheckKeyTtl(key);

        if (!db.Contains(key)) {
            db.Emplace(key, ValueType{-1, db.Factory<DBSet>()});
        }

        if (auto* s = std::get_if<DBSet>(&db.At(key).value)) {
            long long cnt = 0;
            for (size_t i = 1; i < args.size(); ++i) {
                if (!s->contains(args[i])) {
                    cnt++;
                    s->insert(args[i]);
                }
            }
            return cnt;
        }
        else {
            return Error("WRONGTYPE Operation against a key holding the wrong kind of value");
        }
    }

    ValKeyServer::RetType ValKeyServer::SRem(DataBase& db, std::vector<DBString>& args) {
        const Key& key = args[0];
        db.CheckKeyTtl(key);

        if (!db.Contains(key)) {
            return 0ll;
        }
        else if (auto* s = std::get_if<DBSet>(&db.At(key).value)) {
            long long cnt = 0;
            for (size_t i = 1; i < args.size(); ++i) {
                if (s->contains(args[i])) {
                    cnt++;
                    s->erase(args[i]);
                }
            }
            if (s->empty()) {
                db.Erase(key);
            }
            return cnt;
        }
        else {
            return Error("WRONGTYPE Operation against a key holding the wrong kind of value");
        }
    }

    ValKeyServer::RetType ValKeyServer::SIsMember(DataBase& db, std::vector<DBString>& args) {
        const Key& key = args[0];
        db.CheckKeyTtl(key);

        if (!db.Contains(key)) {
            return 0ll;
        }
        else if (auto* s = std::get_if<DBSet>(&db.At(key).value)) {
            return (s->contains(args[1]) ? 1ll : 0ll);
        }
        else {
            return Error("WRONGTYPE Operation against a key holding the wrong kind of value");
        }
    }

    ValKeyServer::RetType ValKeyServer::SMembers(DataBase& db, std::vector<DBString>& args) {
        const Key& key = args[0];
        db.CheckKeyTtl(key);

        RESPArray members;
        
        if (!db.Contains(key)) {
            return members;
        }
        else if (auto* s = std::get_if<DBSet>(&db.At(key).value)) {
            for (const auto& i : *s) {
                members.emplace_back(BulkString{std::string(i)}); 
            }
            return members;
        }
        else {
            return Error("WRONGTYPE Operation against a key holding the wrong kind of value");
        }
    }

    ValKeyServer::RetType ValKeyServer::SCard(DataBase& db, std::vector<DBString>& args) {
        const Key& key = args[0];
        db.CheckKeyTtl(key);

        if (!db.Contains(key)) {
            return 0ll;
        }
        else if (auto* s = std::get_if<DBSet>(&db.At(key).value)) {
            return static_cast<long long>(s->size());
        }
        else {
            return Error("WRONGTYPE Operation against a key holding the wrong kind of value");
        }
    }

    ValKeyServer::RetType ValKeyServer::SUnion(DataBase& db, std::vector<DBString>& args) {
        std::unordered_set<DBString> result;

        for (const Key& key : args) {
            db.CheckKeyTtl(key);
            if (!db.Contains(key)) {
                continue;
            }
            else if (auto* s = get_if<DBSet>(&db.At(key).value)) {
                for (const auto& el : *s) {
                    result.insert(el);
                }
            }
            else {
                return Error("WRONGTYPE Operation against a key holding the wrong kind of value");
            }
        }

        RESPArray res;
        for (auto& el : result) res.emplace_back(BulkString{el.c_str()});
        return res;
    }

    ValKeyServer::RetType ValKeyServer::SInter(DataBase& db, std::vector<DBString>& args) {
        std::unordered_set<DBString> result;

        if (auto* s = std::get_if<DBSet>(&db.At(args[0]).value)) {
            result = std::unordered_set<DBString>(s->begin(), s->end());
        }
        else {
            return Error("WRONGTYPE Operation against a key holding the wrong kind of value");   
        }

        for (const Key& key : args) {
            db.CheckKeyTtl(key);
            if (!db.Contains(key) || result.empty()) {
                return RESPArray{};
            }
            else if (auto* s = std::get_if<DBSet>(&db.At(key).value)) {
                std::erase_if(result, [&s](const DBString& el){
                    return !s->contains(el);
                });
            }
            else {
                return Error("WRONGTYPE Operation against a key holding the wrong kind of value");
            }
        }
        
        RESPArray res;
        for (auto& el : result) res.emplace_back(BulkString{el.c_str()});
        return res;
    }

    ValKeyServer::RetType ValKeyServer::SDiff(DataBase& db, std::vector<DBString>& args) {
        std::unordered_set<DBString> result;
        db.CheckKeyTtl(args[0]);

        if (auto* s = std::get_if<DBSet>(&db.At(args[0]).value)) {
            result = std::unordered_set<DBString>(s->begin(), s->end());
        }
        else {
            return Error("WRONGTYPE Operation against a key holding the wrong kind of value");   
        }

        for (size_t i = 1; i < args.size(); ++i) {
            const Key& key = args[i];
            db.CheckKeyTtl(key);

            if (result.empty()) {
                return RESPArray{};
            }
            else if (auto* s = std::get_if<DBSet>(&db.At(key).value)) {
                std::erase_if(result, [&s](const DBString& el){
                    return s->contains(el);
                });
            }
            else {
                return Error("WRONGTYPE Operation against a key holding the wrong kind of value");
            }
        }

        RESPArray res;
        for (auto& el : result) res.emplace_back(BulkString{el.c_str()});
        return res;
    }

    ValKeyServer::RetType ValKeyServer::SMove(DataBase& db, std::vector<DBString>& args) {
        const Key& src = args[0];
        const Key& dest = args[1];
        db.CheckKeyTtl(src);
        db.CheckKeyTtl(dest);
        
        const Key& memb = args[2];

        if ((db.Contains(src) && !std::get_if<DBSet>(&db.At(src).value)) || 
            (db.Contains(dest) && !std::get_if<DBSet>(&db.At(dest).value))) {
            return Error("WRONGTYPE Operation against a key holding the wrong kind of value");
        }

        if (!db.Contains(src)) {
            return 0ll;
        }

        auto* set_src = std::get_if<DBSet>(&db.At(src).value);

        if (!set_src->contains(memb)) {
            return 0ll;
        }

        set_src->erase(memb);

        if (set_src->empty()) {
            db.Erase(src);
        }

        if (!db.Contains(dest)) {
            db.Emplace(dest, ValueType{-1, db.Factory<DBSet>()});
        }

        auto* set_dest = get_if<DBSet>(&db.At(dest).value);

        set_dest->insert(memb);

        return 1ll;
    }

    //----------------------- GEO -------------------------

    ValKeyServer::RetType ValKeyServer::GeoAdd(DataBase& db, std::vector<DBString>& args) {
        const Key& key = args[0];
        db.CheckKeyTtl(key);

        if ((args.size() - 1) % 3 != 0) {
            return Error("ERR wrong number of arguments for geoadd command");
        }

        if (!db.Contains(key)) {
            db.Emplace(key, ValueType{-1, db.Factory<DBGeo>()});
        }

        if (auto* g = std::get_if<DBGeo>(&db.At(key).value)) {
            std::vector<double> lons;
            std::vector<double> lats;
            std::vector<DBString> members;

            for (size_t i = 1; i < args.size(); i += 3) {
                auto lon = IsCorrectInput<double>(args[i]);
                auto lat = IsCorrectInput<double>(args[i + 1]);
                if (!lon || !lat) {
                    return Error("ERR value is not a valid float");
                }
                if (std::abs(*lon) > 180 || std::abs(*lat) > 85.05112878) {
                    return Error("ERR invalid longitude,latitude pair");
                }
                lons.push_back(*lon);
                lats.push_back(*lat);
                members.emplace_back(std::move(args[i + 2]));
            }

            long long cnt = 0;
            for (size_t i = 0; i < members.size(); ++i) {
                long long score = GetScore(lons[i], lats[i]);
                g->Emplace(score, std::move(members[i]));
                ++cnt;
            }

            return cnt;
        }
        else {
            return Error("WRONGTYPE Operation against a key holding the wrong kind of value");
        }
    }

    ValKeyServer::RetType ValKeyServer::GeoPos(DataBase& db, std::vector<DBString>& args) {
        const Key& key = args[0];
        db.CheckKeyTtl(key);

        if (!db.Contains(key)) {
            return Nil{};
        }
        else if (auto* g = std::get_if<DBGeo>(&db.At(key).value)) {
            RESPArray result;
            for (size_t i = 1; i < args.size(); ++i) {
                if (g->Contains(args[i])) {
                    auto [lon, lat] = GetLonlat(g->At(args[i])->first);
                    result.emplace_back(RESPArray{BulkString{std::format("{:.2f}", lon)}, BulkString{std::format("{:.2f}", lat)}});
                }
                else {
                    result.emplace_back("(nil)");
                }
            }
            return result;
        }
        else {
            return Error("WRONGTYPE Operation against a key holding the wrong kind of value");
        }
    }

    ValKeyServer::RetType ValKeyServer::GeoDist(DataBase& db, std::vector<DBString>& args) {
        const Key& key = args[0];
        db.CheckKeyTtl(key);

        const DBString& memb1 = args[1];
        const DBString& memb2 = args[2];

        std::string unit = "m";
        if (args.size() == 4) {
            if (kUnits.contains(std::string(args[3]))) {
                unit = args[3];
            }
            else {
                return Error("ERR unsupported unit provided. any zip code or distance must be a measure distance");
            }
        }

        if (!db.Contains(key)) {
            return Nil{};
        }
        else if (auto* g = std::get_if<DBGeo>(&db.At(key).value)) {
            if (!g->Contains(memb1) || !g->Contains(memb2)) {
                return Nil{};
            }
            auto [lon1, lat1] = GetLonlat(g->At(memb1)->first);
            auto [lon2, lat2] = GetLonlat(g->At(memb2)->first);

            return BulkString{std::format("{:.2f}", GetDist(lon1, lat1, lon2, lat2, unit))}; 
        }
        else {
            return Error("WRONGTYPE Operation against a key holding the wrong kind of value");
        }
    }

    ValKeyServer::RetType ValKeyServer::GeoSearch(DataBase& db, std::vector<DBString>& args) {
        const Key& key = args[0];
        db.CheckKeyTtl(key);

        auto exp = GeoSearchParse(args, 1);

        if (!exp) {
            return exp.error();
        }

        auto [lon, lat, rad, unit, cnt, sort] = exp.value();

        RESPArray result;

        if (!db.Contains(key)) {
            return result;
        }
        else if (auto* g = get_if<DBGeo>(&db.At(key).value)) {

            std::vector<std::pair<double, DBGeo::ByMember::iterator>> to_sort;
            for (auto it = g->ScoresBegin(); it != g->ScoresEnd(); ++it) {
                auto [lon1, lat1] = GetLonlat(it->first);
                auto dist = GetDist(lon1, lat1, *lon, *lat, *unit);
                if (dist <= *rad)
                    to_sort.emplace_back(dist, g->Find(it->second));
            }

            auto cmp = *sort == "asc" ? 
                [](std::pair<double, DBGeo::ByMember::iterator>& a, std::pair<double, DBGeo::ByMember::iterator>& b) {
                    return a.first < b.first;
                } :
                [](std::pair<double, DBGeo::ByMember::iterator>& a, std::pair<double, DBGeo::ByMember::iterator>& b) {
                    return a.first > b.first;
                };

            std::sort(to_sort.begin(), to_sort.end(), cmp);

            size_t lim = cnt ? std::min(*cnt, to_sort.size()) : to_sort.size();

            for (size_t i = 0; i < lim; ++i) {
                result.emplace_back(BulkString{std::string(to_sort[i].second->first)});
            }

            return result;
        }
        else {
            return Error("WRONGTYPE Operation against a key holding the wrong kind of value");
        }
    }



    ValKeyServer::RetType ValKeyServer::GeoSearchStore(DataBase& db, std::vector<DBString>& args) {
        const Key& dest = args[0];
        const Key& src = args[1];
        db.CheckKeyTtl(dest);
        db.CheckKeyTtl(src);

        auto exp = GeoSearchParse(args, 2);

        if (!exp) {
            return exp.error();
        }

        auto [lon, lat, rad, unit, cnt, sort] = exp.value();

        if (db.Contains(dest)) {
            if (!get_if<DBGeo>(&db.At(dest).value)) {
                return Error("WRONGTYPE Operation against a key holding the wrong kind of value");
            }
        }

        std::vector<std::pair<double, DBGeo::ByMember::iterator>> to_sort;

        if (db.Contains(src)) {
            if (auto* src_p = get_if<DBGeo>(&db.At(src).value)) {
                for (auto it = src_p->ScoresBegin(); it != src_p->ScoresEnd(); ++it) {
                    auto [lon1, lat1] = GetLonlat(it->first);
                    auto dist = GetDist(lon1, lat1, *lon, *lat, *unit);
                    if (dist <= *rad) {
                        to_sort.emplace_back(dist, src_p->Find(it->second));
                    }
                }
            } 
            else {
                return Error("WRONGTYPE Operation against a key holding the wrong kind of value");
            }
        }

        auto cmp = *sort == "asc" ? 
            [](std::pair<double, DBGeo::ByMember::iterator>& a, std::pair<double, DBGeo::ByMember::iterator>& b) {
                return a.first < b.first;
            } :
            [](std::pair<double, DBGeo::ByMember::iterator>& a, std::pair<double, DBGeo::ByMember::iterator>& b) {
                return a.first > b.first;
            };

        std::sort(to_sort.begin(), to_sort.end(), cmp);

        size_t lim = cnt ? std::min(*cnt, to_sort.size()) : to_sort.size();

        if (lim == 0) {
            if (db.Contains(dest)) {
                db.Erase(dest);
            }
            return 0ll;
        }

        if (db.Contains(dest)) {
            db.Erase(dest);
        }
        db.Emplace(dest, ValueType{-1, db.Factory<DBGeo>()});
        DBGeo* dest_p = get_if<DBGeo>(&db.At(dest).value);

        for (size_t i = 0; i < lim; ++i) {
            dest_p->Emplace(to_sort[i].second->second->first, to_sort[i].second->first);
        }

        return static_cast<long long>(dest_p->Size());
    }

    //----------------------- GENERAL -------------------------

    ValKeyServer::RetType ValKeyServer::Expire(TTLQueue& q, DataBase& db, std::vector<DBString>& args) {
        const Key& key = args[0];
        db.CheckKeyTtl(key);

        auto seconds = IsCorrectInput<size_t>(args[1]);

        if (!seconds) {
            return Error("ERR value is not an integer or out of range");
        }

        if (!db.Contains(key)) {
            return 0ll;
        }

        long long expire_time = CurTime() + *seconds;
        db.At(key).ttl = expire_time;
        q.insert({expire_time, key});
        return 1ll;
    }

    ValKeyServer::RetType ValKeyServer::TTL(DataBase& db, std::vector<DBString>& args) {
        const Key& key = args[0];
        db.CheckKeyTtl(key);

        if (!db.Contains(key)) {
            return -2ll;
        }
        else if (db.At(key).ttl == -1) {
            return -1ll;
        }
        else {
            return db.At(key).ttl - CurTime();
        }
    }   

    ValKeyServer::RetType ValKeyServer::Type(DataBase& db, std::vector<DBString>& args) {
        const Key& key = args[0];
        db.CheckKeyTtl(key);

        if (!db.Contains(key)) {
            return "none";
        }
        return std::visit(Overload{
            [](DBString& s) {
                return "string";
            },
            [](DBList& s) {
                return "list";
            },
            [](DBSet& s) {
                return "set";
            },
            [](DBGeo& s) {
                return "geo";
            },
        }, db.At(key).value);
    }   

    ValKeyServer::RetType ValKeyServer::Del(DataBase& db, std::vector<DBString>& args) {
        long long cnt = 0;
        for (const Key& key : args) {
            db.CheckKeyTtl(key);
            if (db.Contains(key)) {
                ++cnt;
                db.Erase(key);
            }
        }
        return cnt;
    }

    ValKeyServer::RetType ValKeyServer::Exists(DataBase& db, std::vector<DBString>& args) {
        long long cnt = 0;
        for (const Key& key : args) {
            db.CheckKeyTtl(key);
            if (db.Contains(key)) {
                ++cnt;
            }
        }
        return cnt;
    }

    ValKeyServer::RetType ValKeyServer::Keys(DataBase& db, std::vector<DBString>& args) {
        const DBString& pattern = args[0];
        RESPArray result;
        std::regex reg(pattern.c_str());
        for (const auto& [key, value] : db) {
            db.CheckKeyTtl(key);
            if (std::regex_match(key.begin(), key.end(), reg)) {
                result.emplace_back(std::string(key));
            }
        }
        return result;
    }

    ValKeyServer::RetType ValKeyServer::FlushDB(DataBase& db, std::vector<DBString>& args) {
        db.Clear();
        return "OK";
    }

    ValKeyServer::RetType ValKeyServer::ConfigSet(DataBase& db, std::vector<DBString>& args) {
        if (!Cmp(args[0], "maxmemory")) {
            return Error("ERR Unknown option or number of arguments for CONFIG SET - " + std::string(args[0]));
        }

        size_t bytes;
        DBString& arg = args[1];
        auto [ptr, ec] = std::from_chars(arg.c_str(), arg.c_str() + arg.size(), bytes);
        if (ec == std::errc::invalid_argument) {
            return Error("ERR value is not an integer or out of range");
        }

        std::string suf(ptr, arg.c_str() + arg.size());

        if (suf == "b") bytes *= 1;
        else if (suf == "kb") bytes *= 1024;
        else if (suf == "mb") bytes *= 1024 * 1024;
        else if (suf == "gb") bytes *= 1024 * 1024 * 1024;
        else {
            return Error("ERR Invalid argument " + std::string(arg) + " for CONFIG SET 'maxmemory'");
        }
        
        db.SetMemLim(bytes);

        return "OK";
    }

    ValKeyServer::RetType ValKeyServer::ConfigGet(DataBase& db, std::vector<DBString>& args) {
        if (!Cmp(args[0], "maxmemory")) {
            return -1ll;
        }
        return RESPArray {std::string(args[0]), static_cast<long long>(db.GetMemLim())};
    }

    ValKeyServer::RetType ValKeyServer::DBsize(DataBase& db, std::vector<DBString>& args) {
        return static_cast<long long>(db.Size());
    }

    ValKeyServer::RetType ValKeyServer::MemoryUsage(DataBase& db, std::vector<DBString>& args) {
        const Key& key = args[0];
        db.CheckKeyTtl(key);

        if (!db.Contains(key)) {
            return Nil{};
        }
        return static_cast<long long>(db.UsageMem(key));
    }

} // namespace val_key
