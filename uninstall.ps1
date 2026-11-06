# find_mymouse_byshake uninstaller.
#
#   irm https://raw.githubusercontent.com/chriscollective/find_mymouse_byshake/main/uninstall.ps1 | iex
#
# Kept ASCII-only so Windows PowerShell 5.1 reads it correctly.

$ErrorActionPreference = 'Stop'

$Name     = 'find_mymouse_byshake'
$Dir      = Join-Path $env:LOCALAPPDATA $Name
$Exe      = Join-Path $Dir "$Name.exe"
$RunKey   = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Run'
$CfgKey   = "HKCU:\Software\$Name"
$Shortcut = Join-Path ([Environment]::GetFolderPath('Programs')) 'Find My Mouse by Shake.lnk'

function Wait-Exit([int]$Tenths) {
    for ($i = 0; $i -lt $Tenths; $i++) {
        if (-not (Get-Process -Name $Name -ErrorAction SilentlyContinue)) { return $true }
        Start-Sleep -Milliseconds 100
    }
    return $false
}

if (Test-Path $Exe) {
    Start-Process -FilePath $Exe -ArgumentList '--quit' -Wait
}
if (-not (Wait-Exit 50)) {
    Get-Process -Name $Name -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue
    if (-not (Wait-Exit 30)) {
        throw 'The running copy could not be closed. Right-click its tray icon, choose Exit, then run this again.'
    }
}

# Always (before the settings key with its recovery flag is deleted):
# if the app was ever killed or crashed mid-effect, the system cursors
# could still be blank. SPI_SETCURSORS (0x57) reloads the user's scheme.
$sig = '[DllImport("user32.dll")] public static extern bool SystemParametersInfo(uint a, uint b, System.IntPtr c, uint d);'
$spi = Add-Type -MemberDefinition $sig -Name 'Spi' -Namespace 'Fmmbs' -PassThru
[void]$spi::SystemParametersInfo(0x57, 0, [IntPtr]::Zero, 0)

Remove-ItemProperty -Path $RunKey -Name $Name -ErrorAction SilentlyContinue
if (Test-Path $CfgKey)   { Remove-Item -Path $CfgKey -Recurse -Force }
if (Test-Path $Shortcut) { Remove-Item -LiteralPath $Shortcut -Force }
if (Test-Path $Dir)      { Remove-Item -LiteralPath $Dir -Recurse -Force }

Write-Host 'find_mymouse_byshake has been removed.' -ForegroundColor Green
