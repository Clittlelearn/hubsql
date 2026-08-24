#include "model/transaction.h"

#include "utils/reflect_struct.h"

namespace hubsql {

nlohmann::json Transaction::ToJson() const {
    nlohmann::json j;
    reflect::Serialize(*this, j);
    return j;
}

}  // namespace hubsql
