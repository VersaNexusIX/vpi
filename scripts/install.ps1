$ErrorActionPreference = "Stop"
$VPI_VERSION = "1.0.0"
$VPI_REPO = "https://versas.my.id/vpi"
$INSTALL_DIR = "$env:LOCALAPPDATA\vpi\bin"
function Write-Info  { Write-Host "[INFO] $args" -ForegroundColor Cyan }
function Write-Ok    { Write-Host "[OK] $args" -ForegroundColor Green }
function Write-Warn  { Write-Host "[WARN] $args" -ForegroundColor Yellow }
function Write-Fail  { Write-Host "[ERROR] $args" -ForegroundColor Red; exit 1 }
Write-Host ""
Write-Host " __ ___ __ _" -ForegroundColor Cyan
Write-Host " \ V / '_ \| |" -ForegroundColor Cyan
Write-Host "  \_/| .__/|_|" -ForegroundColor Cyan
Write-Host "     |_|      v$VPI_VERSION" -ForegroundColor Cyan
Write-Host ""
$ARCH = if ([System.Environment]::Is64BitOperatingSystem) { "amd64" } else { "x86" }
$ARM = (Get-WmiObject Win32_Processor).Architecture
if ($ARM -eq 12) { $ARCH = "arm64" }
Write-Info "Installing VPI v$VPI_VERSION for windows/$ARCH"
$TMP = [System.IO.Path]::GetTempPath() + "vpi_install_" + [System.IO.Path]::GetRandomFileName()
New-Item -ItemType Directory -Path $TMP -Force | Out-Null
try {
    $ZIP_URL = "$VPI_REPO/releases/v$VPI_VERSION/vpi-windows-$ARCH.zip"
    Write-Info "Downloading: $ZIP_URL"
    $ZIP_PATH = "$TMP\vpi.zip"
    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
    try {
        Invoke-WebRequest -Uri $ZIP_URL -OutFile $ZIP_PATH -UseBasicParsing
    } catch {
        Write-Fail "Download failed. Check your internet connection or visit https://versas.my.id"
    }
    Write-Info "Extracting..."
    Expand-Archive -Path $ZIP_PATH -DestinationPath $TMP -Force
    if (!(Test-Path $INSTALL_DIR)) {
        New-Item -ItemType Directory -Path $INSTALL_DIR -Force | Out-Null
    }
    Copy-Item "$TMP\vpi.exe" -Destination "$INSTALL_DIR\vpi.exe" -Force
    Write-Ok "Installed to $INSTALL_DIR\vpi.exe"
    $CURRENT_PATH = [System.Environment]::GetEnvironmentVariable("PATH", "User")
    if ($CURRENT_PATH -notlike "*$INSTALL_DIR*") {
        Write-Info "Adding $INSTALL_DIR to PATH..."
        [System.Environment]::SetEnvironmentVariable("PATH", "$CURRENT_PATH;$INSTALL_DIR", "User")
        $env:PATH = "$env:PATH;$INSTALL_DIR"
        Write-Ok "PATH updated. Restart terminal for changes to take effect."
    }
    Write-Host ""
    Write-Host "Installation complete!" -ForegroundColor Green
    Write-Host "Run: " -NoNewline; Write-Host "vpi help" -ForegroundColor Cyan
    Write-Host ""
} finally {
    Remove-Item -Recurse -Force $TMP -ErrorAction SilentlyContinue
}
