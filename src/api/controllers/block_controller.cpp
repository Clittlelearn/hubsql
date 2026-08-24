#include "api/controllers/block_controller.h"

#include "api/response.h"

namespace hubsql {

BlockController::BlockController(BlockRepo& repo) : repo_(repo) {}

nlohmann::json BlockController::List(uint64_t start, uint64_t end,
                                     int page, int size) {
    try {
        auto total = repo_.Count();
        auto blocks = repo_.List(start, end, page, size);
        nlohmann::json list = nlohmann::json::array();
        for (const auto& b : blocks) {
            list.push_back(b.ToJson());
        }
        return Ok({{"list", list}, {"total", total}, {"page", page}, {"size", size}});
    } catch (const std::exception& e) {
        return Err(500, e.what());
    }
}

nlohmann::json BlockController::Detail(uint64_t height) {
    try {
        auto b = repo_.FindByHeight(height);
        if (!b) return Err(404, "block not found");
        return Ok(b->ToJson());
    } catch (const std::exception& e) {
        return Err(500, e.what());
    }
}

}  // namespace hubsql
