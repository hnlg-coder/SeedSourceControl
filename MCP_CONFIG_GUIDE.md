# Trae + Everything Search MCP 配置指南

## 概述

本文档说明如何配置 Trae CN IDE 使用 Everything Search MCP 服务，实现快速的文件搜索功能。

## 已完成的工作

1. ✅ 已安装 `everything-mcp` NPM 包
2. ✅ 已创建搜索脚本示例文件
3. ⏳ 需要手动配置 Trae MCP 设置

## 配置步骤

### 步骤 1：打开 Trae 设置

1. 打开 Trae CN IDE
2. 点击左下角的设置图标（或使用快捷键 `Ctrl + ,`）
3. 选择 "MCP" 或 "扩展" 设置页面

### 步骤 2：添加 Everything MCP 服务器

在 MCP 配置文件中添加以下内容。配置文件通常位于：
- **Windows**: `C:\Users\Administrator\AppData\Roaming\TRAE SOLO CN\User\mcp.json`
- **macOS**: `~/Library/Application Support/TRAE SOLO CN/User/mcp.json`

添加以下配置：

```json
{
  "mcpServers": {
    "GitHub": {
      "command": "npx",
      "args": [
        "-y",
        "@modelcontextprotocol/server-github"
      ],
      "env": {
        "GITHUB_PERSONAL_ACCESS_TOKEN": "REDACTED"
      },
      "fromGalleryId": "byted-mcp-volcengine.github"
    },
    "Everything": {
      "command": "npx",
      "args": [
        "-y",
        "everything-mcp"
      ],
      "description": "Windows Everything file search with filters for type, size, and date"
    }
  }
}
```

### 步骤 3：重启 Trae

配置完成后，重启 Trae IDE 以加载新的 MCP 服务器。

## 使用方法

配置成功后，您可以使用以下功能：

### 基础搜索
```
搜索所有文件名包含 "test" 的文件
```

### 按文件类型筛选
```
搜索所有 .txt 文件
```

### 按文件大小筛选
```
搜索大于 1MB 的文件
```

### 按修改日期筛选
```
搜索今天修改的文件
```

## 前提条件

确保以下条件满足：

1. **Everything 已安装**
   - 安装路径：`C:\Program Files\Everything\Everything.exe`
   - 已启动并运行

2. **Everything HTTP 服务器已启用**
   - 打开 Everything 设置
   - 勾选 "HTTP 服务器" 选项
   - 端口默认：`25821`

3. **Everything 以管理员权限运行**（推荐）
   - 右键点击 Everything
   - 选择 "以管理员身份运行"
   - 这样可以搜索系统文件和受保护目录

## 故障排除

### 问题 1：MCP 服务器未启动

**解决方案**：
- 检查 Everything 是否正在运行
- 检查 HTTP 服务器是否启用
- 重启 Trae IDE

### 问题 2：搜索结果为空

**解决方案**：
- 确保 Everything 已经建立了文件索引
- 在 Everything 中手动搜索确认索引正常
- 检查搜索查询语法是否正确

### 问题 3：权限不足

**解决方案**：
- 以管理员权限运行 Everything
- 以管理员权限启动 Trae

## 其他资源

- [Everything 官方网站](https://www.voidtools.com/)
- [everything-mcp GitHub](https://github.com/chahero/everything-mcp)
- [Model Context Protocol 文档](https://modelcontextprotocol.io/)

## 技术细节

### Everything MCP 功能

- **基础搜索**：按文件名搜索
- **扩展名筛选**：支持 `.txt`, `.pdf`, `.doc` 等
- **大小筛选**：支持 `size:>1mb`, `size:<100kb` 等
- **日期筛选**：支持 `dm:today`, `dm:2024-01-01` 等
- **路径筛选**：支持 `path:c:\windows` 等

### 搜索语法

Everything 使用类似以下的搜索语法：

```
ext:txt size:>1mb dm:today
```

参数说明：
- `ext`: 文件扩展名
- `size`: 文件大小
- `dm`: 修改日期
- `dc`: 创建日期
- `path`: 文件路径

## 支持

如有问题，请查看：
1. Everything MCP 官方文档
2. Trae IDE MCP 配置文档
3. Everything 软件帮助文档
