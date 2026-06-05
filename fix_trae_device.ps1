# 修复Trae设备ID问题的完整脚本
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Trae设备ID修复工具" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# 第一步：关闭所有Trae相关进程
Write-Host "[1/5] 正在关闭Trae进程..." -ForegroundColor Yellow
$traeProcesses = Get-Process | Where-Object { 
    $_.ProcessName -like "*trae*" -or 
    $_.ProcessName -like "*TRAE*" -or 
    $_.MainWindowTitle -like "*Trae*" 
}
if ($traeProcesses) {
    Write-Host "发现 $($traeProcesses.Count) 个Trae进程，正在关闭..." -ForegroundColor Yellow
    $traeProcesses | Stop-Process -Force -ErrorAction SilentlyContinue
    Start-Sleep -Seconds 3
    Write-Host "Trae进程已关闭" -ForegroundColor Green
} else {
    Write-Host "没有发现运行中的Trae进程" -ForegroundColor Green
}

# 第二步：备份配置
Write-Host ""
Write-Host "[2/5] 正在备份配置..." -ForegroundColor Yellow
$traeAppData = "$env:APPDATA\TRAE SOLO CN"
$backupDir = "$env:APPDATA\TRAE SOLO CN_backup_$(Get-Date -Format 'yyyyMMdd_HHmmss')"
if (Test-Path $traeAppData) {
    Write-Host "正在备份到: $backupDir" -ForegroundColor Cyan
    Copy-Item -Path $traeAppData -Destination $backupDir -Recurse -Force
    Write-Host "备份完成" -ForegroundColor Green
}

# 第三步：清除设备相关的配置
Write-Host ""
Write-Host "[3/5] 正在清除设备配置..." -ForegroundColor Yellow

# 清除Local Storage中的leveldb（设备ID很可能在这里）
$levelDbPath = "$traeAppData\Local Storage\leveldb"
if (Test-Path $levelDbPath) {
    Write-Host "正在清除 Local Storage leveldb..." -ForegroundColor Cyan
    Remove-Item -Path "$levelDbPath\*" -Recurse -Force -ErrorAction SilentlyContinue
    Write-Host "Local Storage leveldb 已清除" -ForegroundColor Green
}

# 清除ahanet目录中的配置
$ahanetPath = "$traeAppData\ahanet"
if (Test-Path $ahanetPath) {
    Write-Host "正在清除 ahanet 配置..." -ForegroundColor Cyan
    Remove-Item -Path "$ahanetPath\*" -Recurse -Force -ErrorAction SilentlyContinue
    Write-Host "ahanet 配置已清除" -ForegroundColor Green
}

# 清除Session Storage
$sessionStoragePath = "$traeAppData\Session Storage"
if (Test-Path $sessionStoragePath) {
    Write-Host "正在清除 Session Storage..." -ForegroundColor Cyan
    Remove-Item -Path "$sessionStoragePath\*" -Recurse -Force -ErrorAction SilentlyContinue
    Write-Host "Session Storage 已清除" -ForegroundColor Green
}

# 清除Local Storage中的其他文件
$localStoragePath = "$traeAppData\Local Storage"
if (Test-Path $localStoragePath) {
    Write-Host "正在清除 Local Storage 其他文件..." -ForegroundColor Cyan
    Get-ChildItem -Path $localStoragePath -File | Remove-Item -Force -ErrorAction SilentlyContinue
    Write-Host "Local Storage 已清除" -ForegroundColor Green
}

# 清除用户本地.nodemid文件（如果存在）
$nodemidPath = "$env:USERPROFILE\.nodemid"
if (Test-Path $nodemidPath) {
    Write-Host "正在重命名 .nodemid 文件..." -ForegroundColor Cyan
    Rename-Item -Path $nodemidPath -NewName "$nodemidPath.old" -Force -ErrorAction SilentlyContinue
    Write-Host ".nodemid 已备份为 .nodemid.old" -ForegroundColor Green
}

# 第四步：清理可能的临时文件
Write-Host ""
Write-Host "[4/5] 正在清理临时文件..." -ForegroundColor Yellow
$tempFiles = @(
    "$traeAppData\IndexedDB",
    "$traeAppData\Code Cache",
    "$traeAppData\GPUCache",
    "$traeAppData\Service Worker",
    "$traeAppData\blob_storage"
)
foreach ($path in $tempFiles) {
    if (Test-Path $path) {
        Write-Host "正在清除: $path" -ForegroundColor Cyan
        Remove-Item -Path $path -Recurse -Force -ErrorAction SilentlyContinue
    }
}
Write-Host "临时文件清理完成" -ForegroundColor Green

# 第五步：完成
Write-Host ""
Write-Host "[5/5] 修复完成！" -ForegroundColor Green
Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "现在请按以下步骤操作：" -ForegroundColor Cyan
Write-Host "1. 手动重新启动 TRAE SOLO CN" -ForegroundColor White
Write-Host "2. 使用您的账号重新登录" -ForegroundColor White
Write-Host "3. 尝试开启移动端控制功能" -ForegroundColor White
Write-Host ""
Write-Host "备份位置: $backupDir" -ForegroundColor Gray
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "按任意键退出..."
$null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
