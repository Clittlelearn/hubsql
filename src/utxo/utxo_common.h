#pragma once

#include <string>

namespace hubsql {

// 虚拟/系统地址（不计入真实余额）
inline bool IsVirtualAddr(const std::string& addr) {
    return addr.rfind("Virtual", 0) == 0 || addr.rfind("LockVirtual", 0) == 0;
}

// 纯燃烧 sink（永不消费，永久销币）
inline bool IsSinkAddr(const std::string& addr) {
    return addr == "VirtualBurnGas" || addr == "VirtualCallFlowOutBurnGas";
}

}  // namespace hubsql
