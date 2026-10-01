#pragma once

#include "ValKeyServer.hpp"

#include <expected>
#include <string>
#include <vector>

namespace val_key {
    std::vector<std::string> RESPParse(const std::string& input);

    std::expected<std::string, std::string> Format(ValKeyServer::RetType& resp);

    int Run(int argc, char** argv);
} // namespace val_key

