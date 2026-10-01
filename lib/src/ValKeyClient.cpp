#include <algorithm>
#include <cctype>
#include <cstddef>
#include <expected>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <variant>
#include <vector>
#include <iomanip> 

#include "Overload.hpp"
#include "ValKeyServer.hpp"

namespace val_key {
    std::vector<std::string> RESPParse(const std::string& input) {
        std::stringstream ss(input);
        std::vector<std::string> tokens;
        std::string word;
        while (ss >> std::quoted(word)) {
            tokens.push_back(std::move(word));
        }
        return tokens;
    }

    std::expected<std::string, std::string> Format(ValKeyServer::RetType& resp, const std::string& indent = "") {
        using ret = std::expected<std::string, std::string>;
        return std::visit(
            Overload {
                [](Nil&) -> ret {
                    return std::string("(nil)\n");
                },
                [](long long num) -> ret {
                    return "(integer) " + std::to_string(num) + '\n';
                },
                [](std::string& str) -> ret {
                    return str + '\n';
                },
                [](BulkString& str) -> ret {
                    return '"' + str.value + "\"\n";
                },
                [](Error& er) -> ret {
                    return std::unexpected<std::string> ("(Error) " + er.What() + '\n');
                },
                [&indent](ValKeyServer::RESPArray& v) -> ret {
                    if (v.empty()) {
                        return std::string("(empty array)\n");
                    }
                    std::string ans = "";
                    for (size_t i = 0; i < v.size(); ++i) {
                        std::string prefix = std::to_string(i + 1) + ") ";
                        std::string next_indent = indent + std::string(prefix.length(), ' ');
                        
                        auto inner_resp = Format(v[i], next_indent);
                        std::string inner_str = inner_resp ? inner_resp.value() : inner_resp.error();
                        
                        if (inner_str.starts_with(next_indent)) {
                            inner_str = inner_str.substr(next_indent.length());
                        }
                        
                        ans += indent + prefix + inner_str;
                    }
                    return ans;
                },
            },
            resp
        );
    }

    int Run(int argc, char** argv) {
        std::ios::sync_with_stdio(false);
        std::cin.tie(0);

        ValKeyServer serv;

        if (argc != 1) {
            std::vector<std::string> tokens = {"config", "set"};
            for (int i = 0; i < argc; ++i) {
                tokens.emplace_back(argv[i]);
            }
            auto resp = serv.Query(tokens);
            if (auto* p = std::get_if<Error>(&resp)) {
                std::cerr << "Invalid arguments";
                return 1;
            }
        }

        while (true) {
            std::string line;
            std::getline(std::cin, line);
            auto tokens = RESPParse(line);

            if (tokens.empty()) { 
                return 0; 
            }
            std::ranges::transform(tokens[0], tokens[0].begin(), tolower);
            if (tokens[0] == "exit") {
                return 0;
            }

            ValKeyServer::RetType resp = serv.Query(tokens);
            auto exp = Format(resp);
            exp ? std::cout << exp.value() : std::cerr << exp.error();
            std::cout.flush();
            std::cerr.flush();
            
        }
    }
} // namespace val_key