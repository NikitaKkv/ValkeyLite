// #include <gtest/gtest.h>
// #include <vector>
// #include <string>
// #include <variant>
// #include <thread>
// #include <chrono>
// #include <algorithm>

// #include <ValKeyServer.hpp>

// class ValKeyServerTest : public ::testing::Test {
// protected:
//     val_key::ValKeyServer server_;
// };

// TEST_F(ValKeyServerTest, StringOperations) {
//     std::vector<std::string> set_args = {"set", "mykey", "hello"};
//     auto set_res = server_.Query(set_args);
//     EXPECT_TRUE(std::holds_alternative<std::string>(set_res));

//     std::vector<std::string> strlen_args = {"strlen", "mykey"};
//     auto strlen_res = server_.Query(strlen_args);
//     EXPECT_TRUE(std::holds_alternative<long long>(strlen_res));
//     EXPECT_EQ(std::get<long long>(strlen_res), 5);

//     std::vector<std::string> append_args = {"append", "mykey", " world"};
//     auto append_res = server_.Query(append_args);
//     EXPECT_TRUE(std::holds_alternative<long long>(append_res));
//     EXPECT_EQ(std::get<long long>(append_res), 11);

//     std::vector<std::string> get_args = {"get", "mykey"};
//     auto get_res = server_.Query(get_args);
//     EXPECT_TRUE(std::holds_alternative<std::string>(get_res));
//     EXPECT_EQ(std::get<std::string>(get_res), "hello world");

//     std::vector<std::string> get_nil_args = {"get", "non_existent"};
//     auto get_nil_res = server_.Query(get_nil_args);
//     EXPECT_TRUE(std::holds_alternative<val_key::Nil>(get_nil_res));

//     std::vector<std::string> wrong_set_args = {"set", "key"};
//     auto wrong_set_res = server_.Query(wrong_set_args);
//     EXPECT_TRUE(std::holds_alternative<val_key::Error>(wrong_set_res));
//     auto err = std::get<val_key::Error>(wrong_set_res);
//     EXPECT_FALSE(err.What().empty());
// }

// TEST_F(ValKeyServerTest, ListOperations) {
//     std::vector<std::string> lpush_args = {"lpush", "mylist", "val1"};
//     auto lpush_res = server_.Query(lpush_args);
//     EXPECT_TRUE(std::holds_alternative<long long>(lpush_res));
//     EXPECT_EQ(std::get<long long>(lpush_res), 1);

//     std::vector<std::string> rpush_args = {"rpush", "mylist", "val2"};
//     auto rpush_res = server_.Query(rpush_args);
//     EXPECT_TRUE(std::holds_alternative<long long>(rpush_res));
//     EXPECT_EQ(std::get<long long>(rpush_res), 2);

//     std::vector<std::string> llen_args = {"llen", "mylist"};
//     auto llen_res = server_.Query(llen_args);
//     EXPECT_TRUE(std::holds_alternative<long long>(llen_res));
//     EXPECT_EQ(std::get<long long>(llen_res), 2);

//     std::vector<std::string> lrange_args = {"lrange", "mylist", "0", "-1"};
//     auto lrange_res = server_.Query(lrange_args);
//     EXPECT_TRUE(std::holds_alternative<val_key::ValKeyServer::RESPArray>(lrange_res));
//     auto list_vec = std::get<val_key::ValKeyServer::RESPArray>(lrange_res);
//     ASSERT_EQ(list_vec.size(), 2);
//     EXPECT_EQ(list_vec[0], "val1");
//     EXPECT_EQ(list_vec[1], "val2");

//     std::vector<std::string> lindex_args = {"lindex", "mylist", "1"};
//     auto lindex_res = server_.Query(lindex_args);
//     EXPECT_TRUE(std::holds_alternative<std::string>(lindex_res));
//     EXPECT_EQ(std::get<std::string>(lindex_res), "val2");

//     std::vector<std::string> lset_args = {"lset", "mylist", "1", "val2_new"};
//     auto lset_res = server_.Query(lset_args);
//     EXPECT_TRUE(std::holds_alternative<std::string>(lset_res));

//     std::vector<std::string> lindex_after_set_args = {"lindex", "mylist", "1"};
//     auto lindex_after_set_res = server_.Query(lindex_after_set_args);
//     EXPECT_EQ(std::get<std::string>(lindex_after_set_res), "val2_new");

//     std::vector<std::string> linsert_args = {"linsert", "mylist", "BEFORE", "val2_new", "inserted_val"};
//     auto linsert_res = server_.Query(linsert_args);
//     EXPECT_TRUE(std::holds_alternative<long long>(linsert_res));
//     EXPECT_EQ(std::get<long long>(linsert_res), 3);

//     std::vector<std::string> lpop_args = {"lpop", "mylist"};
//     auto lpop_res = server_.Query(lpop_args);
//     EXPECT_TRUE(std::holds_alternative<std::string>(lpop_res));
//     EXPECT_EQ(std::get<std::string>(lpop_res), "val1");

//     std::vector<std::string> rpop_args = {"rpop", "mylist"};
//     auto rpop_res = server_.Query(rpop_args);
//     EXPECT_TRUE(std::holds_alternative<std::string>(rpop_res));
//     EXPECT_EQ(std::get<std::string>(rpop_res), "val2_new");
// }

// TEST_F(ValKeyServerTest, SetOperations) {
//     std::vector<std::string> sadd1_args = {"sadd", "set1", "elem1"};
//     auto sadd1_res = server_.Query(sadd1_args);
//     EXPECT_TRUE(std::holds_alternative<long long>(sadd1_res));
//     EXPECT_EQ(std::get<long long>(sadd1_res), 1);

//     std::vector<std::string> sadd2_args = {"sadd", "set1", "elem2"};
//     server_.Query(sadd2_args);

//     std::vector<std::string> sadd3_args = {"sadd", "set2", "elem2"};
//     server_.Query(sadd3_args);
//     std::vector<std::string> sadd4_args = {"sadd", "set2", "elem3"};
//     server_.Query(sadd4_args);

//     std::vector<std::string> scard_args = {"scard", "set1"};
//     auto scard_res = server_.Query(scard_args);
//     EXPECT_TRUE(std::holds_alternative<long long>(scard_res));
//     EXPECT_EQ(std::get<long long>(scard_res), 2);

//     std::vector<std::string> sismember_args = {"sismember", "set1", "elem1"};
//     auto sismember_res = server_.Query(sismember_args);
//     EXPECT_TRUE(std::holds_alternative<long long>(sismember_res));
//     EXPECT_EQ(std::get<long long>(sismember_res), 1);

//     std::vector<std::string> smembers_args = {"smembers", "set1"};
//     auto smembers_res = server_.Query(smembers_args);
//     EXPECT_TRUE(std::holds_alternative<val_key::ValKeyServer::RESPArray>(smembers_res));
//     auto members = std::get<val_key::ValKeyServer::RESPArray>(smembers_res);
//     ASSERT_EQ(members.size(), 2);

//     std::vector<std::string> sunion_args = {"sunion", "set1", "set2"};
//     auto sunion_res = server_.Query(sunion_args);
//     EXPECT_TRUE(std::holds_alternative<val_key::ValKeyServer::RESPArray>(sunion_res));
//     auto union_res = std::get<val_key::ValKeyServer::RESPArray>(sunion_res);
//     ASSERT_EQ(union_res.size(), 3);
//     EXPECT_EQ(union_res[0], "elem1");
//     EXPECT_EQ(union_res[1], "elem2");
//     EXPECT_EQ(union_res[2], "elem3");

//     std::vector<std::string> sinter_args = {"sinter", "set1", "set2"};
//     auto sinter_res = server_.Query(sinter_args);
//     EXPECT_TRUE(std::holds_alternative<val_key::ValKeyServer::RESPArray>(sinter_res));
//     auto inter_res = std::get<val_key::ValKeyServer::RESPArray>(sinter_res);
//     ASSERT_EQ(inter_res.size(), 1);
//     EXPECT_EQ(inter_res[0], "elem2");

//     std::vector<std::string> sdiff_args = {"sdiff", "set1", "set2"};
//     auto sdiff_res = server_.Query(sdiff_args);
//     EXPECT_TRUE(std::holds_alternative<val_key::ValKeyServer::RESPArray>(sdiff_res));
//     auto diff_res = std::get<val_key::ValKeyServer::RESPArray>(sdiff_res);
//     ASSERT_EQ(diff_res.size(), 1);
//     EXPECT_EQ(diff_res[0], "elem1");

//     std::vector<std::string> smove_args = {"smove", "set1", "set2", "elem1"};
//     auto smove_res = server_.Query(smove_args);
//     EXPECT_TRUE(std::holds_alternative<long long>(smove_res));
//     EXPECT_EQ(std::get<long long>(smove_res), 1);

//     std::vector<std::string> srem_args = {"srem", "set2", "elem1"};
//     auto srem_res = server_.Query(srem_args);
//     EXPECT_TRUE(std::holds_alternative<long long>(srem_res));
//     EXPECT_EQ(std::get<long long>(srem_res), 1);
// }

// TEST_F(ValKeyServerTest, GeoOperations) {
//     std::vector<std::string> geoadd_args1 = {"geoadd", "sicily", "13.361389", "38.115556", "Palermo"};
//     auto geoadd_res1 = server_.Query(geoadd_args1);
//     EXPECT_TRUE(std::holds_alternative<long long>(geoadd_res1));
//     EXPECT_EQ(std::get<long long>(geoadd_res1), 1);   

//     std::vector<std::string> geoadd_args2 = {"geoadd", "sicily", "15.087269", "37.502669", "Catania"};
//     server_.Query(geoadd_args2);
//     std::vector<std::string> geodist_args = {"geodist", "sicily", "Palermo", "Catania", "km"};
//     auto geodist_res = server_.Query(geodist_args);
//     EXPECT_TRUE(std::holds_alternative<std::string>(geodist_res));
//     double dist = std::stod(std::get<std::string>(geodist_res));
//     EXPECT_NEAR(dist, 166.27, 1.0);

//     // std::vector<std::string> geopos_args = {"geopos", "sicily", "Palermo"};
//     // auto geopos_res = server_.Query(geopos_args);
//     // EXPECT_TRUE(std::holds_alternative<val_key::ValKeyServer::RESPArray>(geopos_res));
//     // auto pos = std::get<val_key::ValKeyServer::RESPArray>(geopos_res);
//     // ASSERT_EQ(pos.size(), 2);
//     // EXPECT_NEAR(std::stod(pos[0]), 13.361389, 0.001);
//     // EXPECT_NEAR(std::stod(pos[1]), 38.115556, 0.001);

//     std::vector<std::string> geosearch_args = {"geosearch", "sicily", "FROMLONLAT", "15.0", "37.5", "BYRADIUS", "200", "km"};
//     auto geosearch_res = server_.Query(geosearch_args);
//     EXPECT_TRUE(std::holds_alternative<val_key::ValKeyServer::RESPArray>(geosearch_res));
//     auto search_res = std::get<val_key::ValKeyServer::RESPArray>(geosearch_res);
//     EXPECT_FALSE(search_res.empty());

//     std::vector<std::string> geosearchstore_args = {"geosearchstore", "storesicily", "sicily", "FROMLONLAT", "15.0", "37.5", "BYRADIUS", "200", "km"};
//     auto geosearchstore_res = server_.Query(geosearchstore_args);
//     EXPECT_TRUE(std::holds_alternative<long long>(geosearchstore_res));
// }

// TEST_F(ValKeyServerTest, GenericOperations) {
//     std::vector<std::string> set_args = {"set", "tempkey", "tempval"};
//     server_.Query(set_args);

//     std::vector<std::string> exist_args = {"exist", "tempkey"};
//     auto exist_res = server_.Query(exist_args);
//     EXPECT_TRUE(std::holds_alternative<long long>(exist_res));
//     EXPECT_EQ(std::get<long long>(exist_res), 1);

//     std::vector<std::string> type_args = {"type", "tempkey"};
//     auto type_res = server_.Query(type_args);
//     EXPECT_TRUE(std::holds_alternative<std::string>(type_res));
//     EXPECT_EQ(std::get<std::string>(type_res), "string");

//     std::vector<std::string> expire_args = {"expire", "tempkey", "1"};
//     auto expire_res = server_.Query(expire_args);
//     EXPECT_TRUE(std::holds_alternative<long long>(expire_res));
//     EXPECT_EQ(std::get<long long>(expire_res), 1);

//     std::vector<std::string> ttl_args = {"ttl", "tempkey"};
//     auto ttl_res = server_.Query(ttl_args);
//     EXPECT_TRUE(std::holds_alternative<long long>(ttl_res));
//     EXPECT_GT(std::get<long long>(ttl_res), 0);

//     std::this_thread::sleep_for(std::chrono::milliseconds(2000));

//     std::vector<std::string> ttl_expired_args = {"ttl", "tempkey"};
//     auto ttl_expired_res = server_.Query(ttl_expired_args);
//     EXPECT_TRUE(std::holds_alternative<long long>(ttl_expired_res));
//     EXPECT_EQ(std::get<long long>(ttl_expired_res), -2);

//     std::vector<std::string> exist_after_args = {"exist", "tempkey"};
//     auto exist_after_res = server_.Query(exist_after_args);
//     EXPECT_EQ(std::get<long long>(exist_after_res), 0);
// }

// TEST_F(ValKeyServerTest, KeysAndDbOperations) {
//     std::vector<std::string> set_args1 = {"set", "abc1", "1"};
//     server_.Query(set_args1);
//     std::vector<std::string> set_args2 = {"set", "abc2", "2"};
//     server_.Query(set_args2);
//     std::vector<std::string> set_args3 = {"set", "xyz", "3"};
//     server_.Query(set_args3);

//     std::vector<std::string> dbsize_args = {"dbsize"};
//     auto dbsize_res = server_.Query(dbsize_args);
//     EXPECT_TRUE(std::holds_alternative<long long>(dbsize_res));
//     EXPECT_EQ(std::get<long long>(dbsize_res), 3);

//     std::vector<std::string> keys_args = {"keys", "abc.*"};
//     auto keys_res = server_.Query(keys_args);
//     EXPECT_TRUE(std::holds_alternative<val_key::ValKeyServer::RESPArray>(keys_res));
//     auto keys_list = std::get<val_key::ValKeyServer::RESPArray>(keys_res);
//     ASSERT_EQ(keys_list.size(), 2);
//     EXPECT_EQ(keys_list[0], "abc1");
//     EXPECT_EQ(keys_list[1], "abc2");

//     std::vector<std::string> del_args = {"del", "xyz"};
//     auto del_res = server_.Query(del_args);
//     EXPECT_TRUE(std::holds_alternative<long long>(del_res));
//     EXPECT_EQ(std::get<long long>(del_res), 1);

//     std::vector<std::string> flush_args = {"flushdb"};
//     server_.Query(flush_args);

//     std::vector<std::string> dbsize_empty_args = {"dbsize"};
//     auto dbsize_empty_res = server_.Query(dbsize_empty_args);
//     EXPECT_EQ(std::get<long long>(dbsize_empty_res), 0);
// }

// TEST_F(ValKeyServerTest, SubcommandsAndOOM) {
//     std::vector<std::string> config_set_args = {"config", "set", "maxmemory", "100b"};
//     auto config_set_res = server_.Query(config_set_args);
//     EXPECT_TRUE(std::holds_alternative<std::string>(config_set_res));
//     EXPECT_EQ(std::get<std::string>(config_set_res), "OK");

//     std::vector<std::string> config_get_args = {"config", "get", "maxmemory"};
//     auto config_get_res = server_.Query(config_get_args);
//     EXPECT_TRUE(std::holds_alternative<val_key::ValKeyServer::RESPArray>(config_get_res));
//     auto config_get_val = std::get<val_key::ValKeyServer::RESPArray>(config_get_res);
//     ASSERT_EQ(config_get_val.size(), 2);
//     EXPECT_EQ(config_get_val[0], "maxmemory");

//     std::vector<std::string> config_set_small_args = {"config", "set", "maxmemory", "1b"};
//     server_.Query(config_set_small_args);

//     std::vector<std::string> set_initial_args = {"set", "k1", "large_string_value_to_exceed_one_byte"};
//     server_.Query(set_initial_args);

//     std::vector<std::string> mem_usage_args = {"memory", "usage", "k1"};
//     auto mem_usage_res = server_.Query(mem_usage_args);
//     EXPECT_TRUE(std::holds_alternative<long long>(mem_usage_res));
//     EXPECT_GT(std::get<long long>(mem_usage_res), 0);

//     std::vector<std::string> set_oom_args = {"set", "k2", "another_value"};
//     auto set_oom_res = server_.Query(set_oom_args);
//     EXPECT_TRUE(std::holds_alternative<val_key::Error>(set_oom_res));
//     auto err = std::get<val_key::Error>(set_oom_res);
//     EXPECT_EQ(err.What(), "OOM command not allowed when used memory > 1");

//     std::vector<std::string> get_args = {"get", "k1"};
//     auto get_res = server_.Query(get_args);
//     EXPECT_TRUE(std::holds_alternative<std::string>(get_res));
//     EXPECT_EQ(std::get<std::string>(get_res), "large_string_value_to_exceed_one_byte");
// }