# Trae Everything MCP 配置脚本
# 运行此脚本自动配置 Trae 使用 Everything Search

Write-Host "======================================" -ForegroundColor Cyan
Write-Host "Trae + Everything MCP 配置工具" -ForegroundColor Cyan
Write-Host "======================================" -ForegroundColor Cyan
Write-Host ""

# 检查 Everything 是否安装
Write-Host "[1/4] 检查 Everything 安装..." -ForegroundColor Yellow
$everythingPath = "C:\Program Files\Everything\Everything.exe"
if (Test-Path $everythingPath) {
    Write-Host "[OK] Everything 已安装在: $everythingPath" -ForegroundColor Green
    
    # 检查 Everything 进程
    $everythingProcess = Get-Process -Name "Everything" -ErrorAction SilentlyContinue
    if ($everythingProcess) {
        Write-Host "[OK] Everything 正在运行 (PID: $($everythingProcess.Id))" -ForegroundColor Green
    } else {
        Write-Host "[WARN] Everything 未运行" -ForegroundColor Yellow
        Write-Host "[INFO] 请手动启动 Everything" -ForegroundColor Cyan
    }
} else {
    Write-Host "[ERROR] Everything 未安装，请先安装 Everything" -ForegroundColor Red
    Write-Host "[INFO] 下载地址: https://www.voidtools.com/downloads/" -ForegroundColor Cyan
    exit 1
}

# 检查 MCP 包是否安装
Write-Host ""
Write-Host "[2/4] 检查 everything-mcp 包..." -ForegroundColor Yellow
$npmPath = Get-Command npm -ErrorAction SilentlyContinue
if ($npmPath) {
    try {
        $null = npm list -g everything-mcp --depth=0 2>$null
        if ($LASTEXITCODE -eq 0) {
            Write-Host "[OK] everything-mcp 已安装" -ForegroundColor Green
        } else {
            Write-Host "[INFO] 正在安装 everything-mcp..." -ForegroundColor Yellow
            npm install -g everything-mcp
            if ($LASTEXITCODE -eq 0) {
                Write-Host "[OK] everything-mcp 安装成功" -ForegroundColor Green
            }
        }
    } catch {
        Write-Host "[INFO] 正在安装 everything-mcp..." -ForegroundColor Yellow
        npm install -g everything-mcp
    }
} else {
    Write-Host "[ERROR] NPM 未安装，请安装 Node.js" -ForegroundColor Red
    exit 1
}

# 配置文件路径
Write-Host ""
Write-Host "[3/4] 配置 Trae MCP..." -ForegroundColor Yellow
$mcpConfigPath = "$env:APPDATA\TRAE SOLO CN\User\mcp.json"

if (Test-Path $mcpConfigPath) {
    Write-Host "[INFO] 配置文件已存在: $mcpConfigPath" -ForegroundColor Cyan
    
    $configContent = @"
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
"@
    
    $configContent | Out-File -FilePath $mcpConfigPath -Encoding UTF8
    Write-Host "[OK] Trae MCP 配置已更新" -ForegroundColor Green
} else {
    Write-Host "[INFO] 配置文件不存在，创建新配置..." -ForegroundColor Yellow
    
    $configContent = @"
{
  "mcpServers": {
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
"@
    
    # 确保目录存在
    $configDir = Split-Path $mcpConfigPath -Parent
    if (-not (Test-Path $configDir)) {
        New-Item -ItemType Directory -Path $configDir -Force | Out-Null
    }
    
    $configContent | Out-File -FilePath $mcpConfigPath -Encoding UTF8
    Write-Host "[OK] 新配置文件已创建" -ForegroundColor Green
}

# 完成提示
Write-Host ""
Write-Host "[4/4] 完成！" -ForegroundColor Green
Write-Host ""
Write-Host "======================================" -ForegroundColor Green
Write-Host "配置成功！" -ForegroundColor Green
Write-Host "======================================" -ForegroundColor Green
Write-Host ""
Write-Host "下一步操作：" -ForegroundColor Cyan
Write-Host "1. 重启 Trae IDE" -ForegroundColor White
Write-Host "2. 在 Trae 中打开设置 -> MCP" -ForegroundColor White
Write-Host "3. 确认 Everything MCP 服务器已启用" -ForegroundColor White
Write-Host ""
Write-Host "使用方法：" -ForegroundColor Cyan
Write-Host "- 在 Trae 中使用自然语言搜索文件" -ForegroundColor White
Write-Host "- 例如：'搜索桌面上的所有 PDF 文件'" -ForegroundColor White
Write-Host "- 例如：'查找最近一周修改的文档'" -ForegroundColor White
Write-Host ""
Write-Host "详细文档：查看 MCP_CONFIG_GUIDE.md" -ForegroundColor Cyan
Write-Host ""
Write-Host "配置文件位置：" -ForegroundColor Cyan
Write-Host "$mcpConfigPath" -ForegroundColor White
Write-Host ""
