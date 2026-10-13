# PowerShell 一鍵啟動 v4 (Windows)
# 用法（需先裝 Python 3.10+）：irm https://raw.githubusercontent.com/mokzee9z-alt/xiaocaihua/main/install.ps1 | iex
# v4：Hermes 本體已內含（hermes_src.tar.gz），不用另外安裝 Hermes。
$ErrorActionPreference = "Stop"
$GithubRaw  = "https://raw.githubusercontent.com/mokzee9z-alt/xiaocaihua/main"
$SrcTar     = "hermes_src.tar.gz"
$BackupEnc  = "hermes_backup.tar.gz.enc"
$BackupPass = if ($env:BACKUP_PASS) { $env:BACKUP_PASS } else { "xiaocaihua-2026" }

$HermesHome = if ($env:HERMES_HOME) { $env:HERMES_HOME } else { "$env:USERPROFILE\.hermes" }
$InstallDir = if ($env:INSTALL_DIR) { $env:INSTALL_DIR } else { "$env:USERPROFILE\.local\share\hermes-agent" }

Write-Host "🍅 小菜花 一鍵啟動 v4 (Windows)" -ForegroundColor Cyan
Write-Host "=========================================="
Write-Host "  Hermes 裝到 : $InstallDir"
Write-Host "  設定/記憶在 : $HermesHome"

$tmp = "$env:TEMP\xiaocaihua_setup"
New-Item -ItemType Directory -Force -Path $tmp | Out-Null

Write-Host "[1/6] 檢查 Python..."
$py = (Get-Command py -ErrorAction SilentlyContinue)
if ($py) { $PyExe = "py"; $PyArgs = @("-3") } else {
  $py3 = (Get-Command python -ErrorAction SilentlyContinue)
  if (-not $py3) { Write-Host "❌ 請先安裝 Python 3.10+（安裝時勾 Add to PATH）" -ForegroundColor Red; exit 1 }
  $PyExe = "python"; $PyArgs = @()
}
Write-Host "      -> $(& $PyExe @PyArgs --version)"

Write-Host "[2/6] 下載 Hermes 原始碼 + 備份..."
Invoke-WebRequest -Uri "$GithubRaw/$SrcTar"    -OutFile "$tmp\$SrcTar"
Invoke-WebRequest -Uri "$GithubRaw/$BackupEnc" -OutFile "$tmp\$BackupEnc"
Write-Host "      -> $([math]::Round((Get-Item "$tmp\$SrcTar").Length/1MB,1)) MB + $([math]::Round((Get-Item "$tmp\$BackupEnc").Length/1MB,1)) MB"

Write-Host "[3/6] 解出 Hermes..."
New-Item -ItemType Directory -Force -Path $InstallDir | Out-Null
tar -xzf "$tmp\$SrcTar" -C $InstallDir
Write-Host "      -> $InstallDir"

Write-Host "[4/6] 建虛擬環境並安裝依賴（第一次約 2–5 分鐘）..."
& $PyExe @PyArgs -m venv "$InstallDir\.venv"
$VPy = "$InstallDir\.venv\Scripts\python.exe"
& $VPy -m pip install -q --upgrade pip
& $VPy -m pip install -q -e $InstallDir
Write-Host "      -> OK"

Write-Host "[5/6] 還原記憶 / 技能 / 設定..."
$gz = "$tmp\hermes_backup.tar.gz"
$openssl = (Get-Command openssl -ErrorAction SilentlyContinue)
if ($openssl) {
  & openssl enc -d -aes-256-cbc -pbkdf2 -pass "pass:$BackupPass" -in "$tmp\$BackupEnc" -out $gz
} else {
  # 純 PowerShell 解密（openssl 格式：Salted__ + 8 bytes salt + AES-256-CBC/PKCS7，PBKDF2-SHA256 10000 次）
  $raw = [IO.File]::ReadAllBytes("$tmp\$BackupEnc")
  $salt = $raw[8..15]
  $cipher = $raw[16..($raw.Length-1)]
  $kdf = New-Object System.Security.Cryptography.Rfc2898DeriveBytes($BackupPass, $salt, 10000, [System.Security.Cryptography.HashAlgorithmName]::SHA256)
  $aes = [System.Security.Cryptography.Aes]::Create()
  $aes.Mode = [System.Security.Cryptography.CipherMode]::CBC
  $aes.Padding = [System.Security.Cryptography.PaddingMode]::PKCS7
  $aes.Key = $kdf.GetBytes(32)
  $aes.IV  = $kdf.GetBytes(16)
  $dec = $aes.CreateDecryptor()
  $plain = $dec.TransformFinalBlock($cipher, 0, $cipher.Length)
  [IO.File]::WriteAllBytes($gz, $plain)
}
New-Item -ItemType Directory -Force -Path $HermesHome | Out-Null
tar -xzf $gz -C $HermesHome
if (Test-Path "$HermesHome\hermes") {
  Get-ChildItem "$HermesHome\hermes" -Force | Move-Item -Destination $HermesHome -Force
  Remove-Item "$HermesHome\hermes" -Recurse -Force
}

Write-Host "[6/6] 驗證..."
$mem = if (Test-Path "$HermesHome\memories\MEMORY.md") { (Get-Item "$HermesHome\memories\MEMORY.md").Length } else { 0 }
$skills = if (Test-Path "$HermesHome\skills") { (Get-ChildItem "$HermesHome\skills" -Recurse -Filter SKILL.md).Count } else { 0 }
$ver = (& $VPy -m hermes_agent --version 2>$null | Select-Object -First 1)
Write-Host "==========================================" -ForegroundColor Cyan
if ($mem -gt 500 -and $skills -gt 10) {
  Write-Host "✅ 好了：記憶 $mem bytes / 技能 $skills 個" -ForegroundColor Green
  Write-Host ""
  Write-Host "開始用（把她的 bin 加進 PATH，或直接用完整路徑）："
  Write-Host "  & `"$InstallDir\.venv\Scripts\hermes.exe`" gateway start"
  Write-Host "  & `"$InstallDir\.venv\Scripts\hermes.exe`" chat"
} else {
  Write-Host "❌ 還原不完整（記憶 $mem bytes／技能 $skills）" -ForegroundColor Red
  exit 1
}
Remove-Item $tmp -Recurse -Force
