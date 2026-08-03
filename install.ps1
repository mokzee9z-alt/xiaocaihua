# PowerShell 版一鍵還原 v2.0 (Windows, 加密版)
# 用法: irm <URL>/install.ps1 | iex
$ErrorActionPreference = "Stop"
$GithubRaw = "https://raw.githubusercontent.com/mokzee9z-alt/xiaocaihua/main"
$BackupEnc = "hermes_backup.tar.gz.enc"
$BackupPass = if ($env:BACKUP_PASS) { $env:BACKUP_PASS } else { "xiaocaihua-2026" }

Write-Host "🍅 小菜花還原程式 v2.0 (Windows)" -ForegroundColor Cyan
Write-Host "=========================================="

$HermesHome = if ($env:HERMES_HOME) { $env:HERMES_HOME } else { "$env:USERPROFILE\.hermes" }

Write-Host "[1/4] 下載備份 (加密)..."
$tmp = "$env:TEMP\xiaocaihua_restore"
New-Item -ItemType Directory -Force -Path $tmp | Out-Null
Invoke-WebRequest -Uri "$GithubRaw/$BackupEnc" -OutFile "$tmp\$BackupEnc"
Write-Host "      -> $((Get-Item "$tmp\$BackupEnc").Length / 1MB) MB"

Write-Host "[2/4] 解密..."
openssl enc -d -aes-256-cbc -pbkdf2 -pass pass:"$BackupPass" -in "$tmp\$BackupEnc" -out "$tmp\hermes_backup.tar.gz"
Write-Host "      -> OK"

Write-Host "[3/4] 解壓縮 + 套用..."
New-Item -ItemType Directory -Force -Path $HermesHome | Out-Null
tar -xzf "$tmp\hermes_backup.tar.gz" -C $HermesHome
if (Test-Path "$HermesHome\hermes") {
    Get-ChildItem "$HermesHome\hermes" -Force | Move-Item -Destination $HermesHome -Force
    Remove-Item "$HermesHome\hermes" -Recurse -Force
}

Write-Host "[4/4] 驗證..."
if ((Test-Path "$HermesHome\memories\MEMORY.md") -and (Test-Path "$HermesHome\config.yaml")) {
    Write-Host "==========================================" -ForegroundColor Cyan
    Write-Host "✅ 還原完成! 小菜花已就位" -ForegroundColor Green
    $mem = (Get-Item "$HermesHome\memories\MEMORY.md").Length
    $skills = (Get-ChildItem "$HermesHome\skills" -Recurse -Filter SKILL.md).Count
    Write-Host "   - 記憶: $mem bytes"
    Write-Host "   - 技能: $skills 個"
    Write-Host "   - 設定: config.yaml"
} else {
    Write-Host "❌ 還原失敗, 檔案不完整" -ForegroundColor Red
    exit 1
}
Remove-Item $tmp -Recurse -Force
