# 修复Trae设备ID问题的脚本
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Trae设备ID修复工具" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# 第一步：关闭所有Trae相关进程
Write-Host "[1/4] 正在关闭Trae进程..." -ForegroundColor Yellow
$traeProcesses = Get-Process | Where-Object { $_.ProcessName -like "*trae*" -or $_.ProcessName -like "*TRAE*" -or $_.MainWindowTitle -like "*Trae*" }
if ($traeProcesses) {
    Write-Host "发现 $($traeProcesses.Count) 个Trae进程" -ForegroundColor Yellow
    $traeProcesses | Stop-Process -Force -ErrorAction SilentlyContinue
    Start-Sleep -Seconds 3
    Write-Host "Trae进程已关闭" -ForegroundColor Green
} else {
    Write-Host "没有发现运行中的Trae进程" -ForegroundColor Green
}

# 第二步：备份并清除设备相关配置
Write-Host ""
Write-Host "[2/4] 正在清除设备注册信息..." -ForegroundColor Yellow
$traeAppData = "$env:APPDATA\TRAE SOLO CN"
$backupDir = "$env:APPDATA\TRAE SOLO CN_backup_$(Get-Date -Format 'yyyyMMdd_HHmmss')"

if (Test-Path $traeAppData) {
    # 备份整个目录
    Write-Host "正在备份配置到: $backupDir" -ForegroundColor Cyan
    Copy-Item -Path $traeAppData -Destination $backupDir -Recurse -Force
    
    # 清除Local Storage中的leveldb（设备ID很可能在这里）
    $levelDbPath = "$traeAppData\Local Storage\leveldb"
    if (Test-Path $levelDbPath) {
        Write-Host "正在清除Local Storage leveldb..." -ForegroundColor Cyan
        Remove-Item -Path "$levelDbPath\*" -Recurse -Force -ErrorAction SilentlyContinue
    }
    
    # 清除ahanet目录下的配置
    $ahanetPath = "$traeAppData\ahanet"
    if (Test-Path $ahanetPath) {
        Write-Host "正在清除ahanet配置..." -ForegroundColor Cyan
        Remove-Item -Path "$ahanetPath\*" -Recurse -Force -ErrorAction SilentlyContinue
    }
    
    # 清除session storage
    $sessionStoragePath = "$traeAppData\Session Storage"
    if (Test-Path $sessionStoragePath) {
        Write-Host "正在清除Session Storage..." -ForegroundColor Cyan
        Remove-Item -Path "$sessionStoragePath\*" -Recurse -Force -ErrorAction SilentlyContinue
    }
    
    # 清除网络缓存
    $networkPath = "$traeAppData\Network"
    if (Test-Path $networkPath) {
        Write-Host "正在清除Network缓存..." -ForegroundColor Cyan
        Remove-Item -Path "$networkPath\*" -Recurse -Force -ErrorAction SilentlyContinue
    }
    
    Write-Host "设备注册信息已清除" -ForegroundColor Green
} else {
    Write-Host "未找到Trae配置目录" -ForegroundColor Red
    exit 1
}

# 第三步：提示用户重新启动Trae
Write-Host ""
Write-Host "[3/4] 准备完成！" -ForegroundColor Green
Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "下一步操作：" -ForegroundColor Cyan
Write-Host "1. 手动重新启动Trae" -ForegroundColor White
Write-Host "2. 使用你的账号重新登录" -ForegroundColor White
Write-Host "3. 再次尝试开启移动端控制功能" -ForegroundColor White
Write-Host ""
Write-Host "备份位置：$backupDir" -ForegroundColor Gray
Write-Host "========================================" -ForegroundColor Cyan
