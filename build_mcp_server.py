#!/usr/bin/env python3
"""
MCP Build Server — 在本地 Windows 上编译+测试种子源模块控制器项目
通过 MCP 协议暴露 build / test 工具供 Reasonix Code 调用
"""

import json
import subprocess
import sys
import os
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent
BUILD_DIR = PROJECT_ROOT / "build"

# ── MCP 协议工具定义 ──────────────────────────────────────────────

TOOLS = [
    {
        "name": "build_project",
        "description": "使用 CMake + MSVC 编译项目（配置 + 构建两步）",
        "inputSchema": {
            "type": "object",
            "properties": {
                "config": {
                    "type": "string",
                    "enum": ["Debug", "Release"],
                    "description": "构建配置",
                    "default": "Debug"
                },
                "clean_first": {
                    "type": "boolean",
                    "description": "是否先清理再构建",
                    "default": False
                }
            }
        }
    },
    {
        "name": "run_tests",
        "description": "运行测试可执行文件（TestSimulation）",
        "inputSchema": {
            "type": "object",
            "properties": {
                "config": {
                    "type": "string",
                    "enum": ["Debug", "Release"],
                    "description": "测试配置",
                    "default": "Debug"
                }
            }
        }
    },
    {
        "name": "quick_compile",
        "description": "只编译（跳过 cmake 配置，假设已 configure 过）",
        "inputSchema": {
            "type": "object",
            "properties": {
                "config": {
                    "type": "string",
                    "enum": ["Debug", "Release"],
                    "description": "构建配置",
                    "default": "Debug"
                }
            }
        }
    }
]

# ── 工具实现 ──────────────────────────────────────────────────────

def run_command(cmd, cwd=None, timeout=180):
    """运行命令并返回 stdout + stderr"""
    try:
        result = subprocess.run(
            cmd,
            cwd=cwd or str(PROJECT_ROOT),
            capture_output=True,
            text=True,
            timeout=timeout,
            shell=True
        )
        output = result.stdout
        if result.stderr:
            output += "\n--- stderr ---\n" + result.stderr
        return output, result.returncode
    except subprocess.TimeoutExpired:
        return f"命令超时（>{timeout}s）", -1
    except FileNotFoundError as e:
        return f"找不到命令: {e}", -1

def handle_build_project(args):
    config = args.get("config", "Debug")
    clean_first = args.get("clean_first", False)

    lines = []

    # Step 1: cmake configure
    lines.append(f"=== CMake Configure ({config}) ===")
    BUILD_DIR.mkdir(exist_ok=True)
    qt6_dir = "D:/Qt6.9.1/6.5.3/msvc2019_64/lib/cmake/Qt6"
    cmake_cmd = f'cmake .. -DCMAKE_BUILD_TYPE={config} -DQt6_DIR="{qt6_dir}"'
    out, code = run_command(cmake_cmd, cwd=str(BUILD_DIR))
    lines.append(out)
    if code != 0:
        lines.append(f"❌ CMake configure 失败 (exit={code})")
        return "\n".join(lines), code

    # Step 2: clean if requested
    if clean_first:
        lines.append(f"\n=== Clean ===")
        out, code = run_command(f'cmake --build . --target clean --config {config}', cwd=str(BUILD_DIR))
        lines.append(out)

    # Step 3: build
    lines.append(f"\n=== Build ({config}) ===")
    out, code = run_command(f'cmake --build . --config {config}', cwd=str(BUILD_DIR))
    lines.append(out)
    if code == 0:
        lines.append(f"\n✅ Build success ({config})")
    else:
        lines.append(f"\n❌ Build failed (exit={code})")

    return "\n".join(lines), code

def handle_run_tests(args):
    config = args.get("config", "Debug")
    test_exe = str(BUILD_DIR / config / "TestSimulation.exe")

    if not os.path.exists(test_exe):
        return f"❌ 测试可执行文件不存在: {test_exe}\n请先 build_project", -1

    lines = []
    lines.append(f"=== Running Tests ({test_exe}) ===")
    out, code = run_command(test_exe, cwd=str(PROJECT_ROOT))
    lines.append(out)
    if code == 0:
        lines.append(f"\n✅ All tests passed")
    else:
        lines.append(f"\n❌ Some tests failed (exit={code})")

    return "\n".join(lines), code

def handle_quick_compile(args):
    config = args.get("config", "Debug")
    lines = []
    lines.append(f"=== Quick Compile ({config}) ===")
    out, code = run_command(f'cmake --build . --config {config}', cwd=str(BUILD_DIR))
    lines.append(out)
    if code == 0:
        lines.append(f"\n✅ Compile success ({config})")
    else:
        lines.append(f"\n❌ Compile failed (exit={code})")
    return "\n".join(lines), code

# ── MCP 协议处理 ─────────────────────────────────────────────────

def handle_request(request):
    method = request.get("method", "")
    params = request.get("params", {})
    req_id = request.get("id", None)

    resp = {"jsonrpc": "2.0", "id": req_id}

    if method == "initialize":
        resp["result"] = {
            "protocolVersion": "0.1.0",
            "capabilities": {
                "tools": {}
            },
            "serverInfo": {
                "name": "seed-source-build-server",
                "version": "1.0.0"
            }
        }

    elif method == "tools/list":
        resp["result"] = {"tools": TOOLS}

    elif method == "tools/call":
        tool_name = params.get("name", "")
        arguments = params.get("arguments", {})

        if tool_name == "build_project":
            text, code = handle_build_project(arguments)
        elif tool_name == "run_tests":
            text, code = handle_run_tests(arguments)
        elif tool_name == "quick_compile":
            text, code = handle_quick_compile(arguments)
        else:
            resp["error"] = {"code": -32601, "message": f"Unknown tool: {tool_name}"}
            return resp

        # 把 exit code 放在 isError 标记里
        is_error = code != 0
        resp["result"] = {
            "content": [{"type": "text", "text": text}],
            "isError": is_error
        }

    elif method == "notifications/initialized":
        # no response needed
        return None

    else:
        resp["error"] = {"code": -32601, "message": f"Unknown method: {method}"}

    return resp

def main():
    """Stdin/stdout JSON-RPC loop"""
    while True:
        try:
            line = sys.stdin.readline()
            if not line:
                break
            request = json.loads(line.strip())
            response = handle_request(request)
            if response is not None:
                print(json.dumps(response), flush=True)
        except json.JSONDecodeError as e:
            error_resp = {
                "jsonrpc": "2.0",
                "error": {"code": -32700, "message": f"Parse error: {e}"}
            }
            print(json.dumps(error_resp), flush=True)
        except EOFError:
            break
        except Exception as e:
            error_resp = {
                "jsonrpc": "2.0",
                "error": {"code": -32603, "message": f"Internal error: {e}"}
            }
            print(json.dumps(error_resp), flush=True)

if __name__ == "__main__":
    main()
