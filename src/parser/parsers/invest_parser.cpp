#include "parser/parsers/invest_parser.h"

#include "parser/parsers/parser_util.h"

namespace hubsql {

std::vector<InvestmentRecord> InvestParser::Parse(const Transaction& tx) {
    if (std::to_string(tx.type) != GetTxType()) return {};

    InvestmentRecord rec;
    rec.tx_hash  = tx.hash;
    rec.address  = FirstRealAddr(tx);
    rec.amount   = FirstRealValue(tx);
    return {rec};
}

}  // namespace hubsql
