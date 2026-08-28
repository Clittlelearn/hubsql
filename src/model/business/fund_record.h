#pragma once

#include <cstdint>
#include <string>

#include <nlohmann/json.hpp>

namespace hubsql {

// ERC20 资金发放记录（type=8，transfer(address,uint256)）
struct FundRecord {
    std::string tx_hash;
    uint64_t block_height{0};
    std::string sender;
    std::string recipient;
    std::string contract_address;
    std::string amount;
    uint64_t time{0};

    nlohmann::json ToJson() const;
};

}  // namespace hubsql

