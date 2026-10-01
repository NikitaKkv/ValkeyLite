#pragma once

#include <cctype>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <expected>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include "Allocator.hpp"
#include "DBTypes.hpp"
#include "DataBase.hpp"


namespace val_key {
    struct Nil {};

    class Error  {
    public:
        Error(const std::string& err);
        std::string What();
    private:
        std::string err_;
    };

    struct BulkString {
        std::string value;
    };

    class ValKeyServer {
    public:

        struct RESPArray;

        using RetType = std::variant<
            Nil, 
            long long, 
            std::string, 
            BulkString, 
            RESPArray, 
            Error
        >;

        struct RESPArray : std::vector<RetType> {
            using std::vector<RetType>::vector;
        };

        struct Command {
            RetType (*com)(DataBase&, std::vector<DBString>&);
            size_t min_arg_cnt;
            int max_arg_cnt;
            bool is_increasing;
        };

        ValKeyServer();

        RetType Query(std::vector<std::string>& args);

    private:
        TTLQueue q_;
        DataBase db_;

        std::unordered_map<std::string, Command> commands_;
        std::unordered_map<std::string, 
            std::unordered_map<std::string, Command>> subcommands_;


        //----------------------- Special Class -------------------------

        struct Point {
            double lon;
            double lat;
            CountingString name;
        };

        struct GeoSearchParseResult {
            std::optional<double> lon = std::nullopt;
            std::optional<double> lat = std::nullopt;
            std::optional<double> rad = std::nullopt;
            std::optional<std::string> unit = std::nullopt;
            std::optional<size_t> cnt = std::nullopt;
            std::optional<std::string> sort = std::nullopt;
        };

        static const std::map<std::string, double> kUnits;

        //----------------------- Special -------------------------
        static void ClearDB(DataBase& db, TTLQueue& q);

        static double GetDist(double lon1, double lat1, double lon2, double lat2, const std::string& unit);

        template <typename T>
        static std::optional<T> IsCorrectInput(std::string_view str) {
            T value;
            auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value);
            if (ec == std::errc{} && ptr == str.data() + str.size()) {
                return value;
            }
            return std::nullopt;
        }

        static bool Cmp (const DBString& s1, const std::string& s2);

        static std::expected<GeoSearchParseResult, Error> GeoSearchParse 
            (const std::vector<DBString>& args, size_t start);

        static long long GetScore(double lon, double lat);

        static std::pair<double, double> GetLonlat(long long score);

    //----------------------- STRING -------------------------
        static RetType Set(DataBase& db, std::vector<DBString>& args);

        static RetType Get(DataBase& db, std::vector<DBString>& args);

        static RetType Strlen(DataBase& db, std::vector<DBString>& args);

        static RetType Append(DataBase& db, std::vector<DBString>& args);

        //----------------------- LIST -------------------------

        static RetType LPush(DataBase& db, std::vector<DBString>& args);

        static RetType RPush(DataBase& db, std::vector<DBString>& args);

        static RetType LPop(DataBase& db, std::vector<DBString>& args);

        static RetType RPop(DataBase& db, std::vector<DBString>& args);

        static RetType LLen(DataBase& db, std::vector<DBString>& args);

        static RetType LRange(DataBase& db, std::vector<DBString>& args);

        static RetType LIndex(DataBase& db, std::vector<DBString>& args);

        static RetType LSet(DataBase& db, std::vector<DBString>& args);

        static RetType LInsert(DataBase& db, std::vector<DBString>& args);

        //----------------------- SET -------------------------

        static RetType SAdd(DataBase& db, std::vector<DBString>& args);

        static RetType SRem(DataBase& db, std::vector<DBString>& args);

        static RetType SIsMember(DataBase& db, std::vector<DBString>& args);

        static RetType SMembers(DataBase& db, std::vector<DBString>& args);

        static RetType SCard(DataBase& db, std::vector<DBString>& args);

        static RetType SUnion(DataBase& db, std::vector<DBString>& args);

        static RetType SInter(DataBase& db, std::vector<DBString>& args);

        static RetType SDiff(DataBase& db, std::vector<DBString>& args);

        static RetType SMove(DataBase& db, std::vector<DBString>& args);

        //----------------------- GEO -------------------------

        static RetType GeoAdd(DataBase& db, std::vector<DBString>& args);

        static RetType GeoPos(DataBase& db, std::vector<DBString>& args);

        static RetType GeoDist(DataBase& db, std::vector<DBString>& args);

        static RetType GeoSearch(DataBase& db, std::vector<DBString>& args);

        static RetType GeoSearchStore(DataBase& db, std::vector<DBString>& args);

        //----------------------- GENERAL -------------------------

        static RetType Expire(TTLQueue& q, DataBase& db, std::vector<DBString>& args);

        static RetType TTL(DataBase& db, std::vector<DBString>& args);

        static RetType Type(DataBase& db, std::vector<DBString>& args);

        static RetType Del(DataBase& db, std::vector<DBString>& args);

        static RetType Exists(DataBase& db, std::vector<DBString>& args);

        static RetType Keys(DataBase& db, std::vector<DBString>& args);

        static RetType FlushDB(DataBase& db, std::vector<DBString>& args);

        static RetType ConfigSet(DataBase& db, std::vector<DBString>& args);

        static RetType ConfigGet(DataBase& db, std::vector<DBString>& args);

        static RetType DBsize(DataBase& db, std::vector<DBString>& args);

        static RetType MemoryUsage(DataBase& db, std::vector<DBString>& args);
    };
} // namespace val_key
