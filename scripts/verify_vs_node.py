#!/usr/bin/env python3
"""对比我们 account_balances 与节点 GetBalance。"""
import json, urllib.request

ADDRS = [
    "0x755Ccf704E17570b64E247f0794314e4C8E542CA",
    "0x2E08bA82dcF52966dF03Aa0fd830aFAb70d8911d",
    "0x03bEE3dB52E736B6422DaC4668CC784d05cb2e63",
    "0x97c0620661F230e5C9CeA46B5492a498cC6cA006",
    "0x8F055beCF0fA0A732344AFC8fB598b9401363724",
    "0x458e53542299BeC018A6118c7544C44d3b8911Df",
    "0x26e733E5481c079Abf859DDA613931298C9f0251",
    "0x072DB292dC387760aF7c83A7c2d88CAC0846feE7",
]

def rpc(method, params):
    body = {"id": "1", "jsonrpc": "2.0", "method": method, "params": params}
    req = urllib.request.Request(
        "http://127.0.0.1:13134/" + method,
        data=json.dumps(body).encode(),
        headers={"Content-Type": "application/json"},
    )
    return json.loads(urllib.request.urlopen(req, timeout=20).read())

# 我们 MySQL 中的余额
import subprocess
sql = "SELECT address, balance FROM account_balances;"
out = subprocess.run(
    ["./deploy/mysql-portable/bin/mysql", "-uhubsql", "-phubsql123456",
     "-h127.0.0.1", "-P3306", "hubsql", "-N", "-e", sql],
    capture_output=True, text=True).stdout.strip().splitlines()
ours = {line.split("\t")[0]: int(line.split("\t")[1]) for line in out}

allok = True
for a in ADDRS:
    try:
        node = rpc("GetBalance", {"addr": a, "asset_type": "OHI"})
        nodev = int(node["result"]["balance"])
    except Exception as e:
        nodev = f"ERR {e}"
    mv = ours.get(a)
    ok = (isinstance(nodev, int) and mv == nodev)
    allok &= ok
    print(f"{a[:44]:44} 我们={mv}  节点={nodev}  {'✅' if ok else '❌'}")
print("\n✅ 与节点 GetBalance 全部一致！" if allok else "\n❌ 有差异")
