#pragma once

#include <vector>

#include "model/business/fund_record.h"
#include "model/transaction.h"

namespace hubsql {

class FundParser {
public:
    std::vector<FundRecord> Parse(const Transaction& tx);
};

}  // namespace hubsql

