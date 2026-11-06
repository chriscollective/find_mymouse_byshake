# find_mymouse_byshake installer (per-user, no admin needed).
#
#   irm https://raw.githubusercontent.com/chriscollective/find_mymouse_byshake/main/install.ps1 | iex
#
# Installs to %LOCALAPPDATA%\find_mymouse_byshake, adds a Start Menu
# shortcut, enables "start with Windows" and launches the app.
# Set FMMBS_FROM to a local exe path to install that file instead of
# downloading the latest release (used by CI).
# Kept ASCII-only so Windows PowerShell 5.1 reads it correctly.

$ErrorActionPreference = 'Stop'

$Repo     = 'chriscollective/find_mymouse_byshake'
$Name     = 'find_mymouse_byshake'
$Dir      = Join-Path $env:LOCALAPPDATA $Name
$Exe      = Join-Path $Dir "$Name.exe"
$Url      = "https://github.com/$Repo/releases/latest/download/$Name.exe"
$RunKey   = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Run'
$Shortcut = Join-Path ([Environment]::GetFolderPath('Programs')) 'Find My Mouse by Shake.lnk'

function Wait-Exit([int]$Tenths) {
    for ($i = 0; $i -lt $Tenths; $i++) {
        if (-not (Get-Process -Name $Name -ErrorAction SilentlyContinue)) { return $true }
        Start-Sleep -Milliseconds 100
    }
    return $false
}

function Stop-Running {
    if (Test-Path $Exe) {
        Start-Process -FilePath $Exe -ArgumentList '--quit' -Wait
    }
    if (Wait-Exit 50) { return }
    Get-Process -Name $Name -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue
    if (-not (Wait-Exit 30)) {
        throw 'The running copy could not be closed. Right-click its tray icon, choose Exit, then run this again.'
    }
}

# Downloads (or copies) to a temp file next to the target; returns its path.
function Get-App([string]$Target) {
    $tmp = "$Target.download"
    if ($env:FMMBS_FROM) {
        Copy-Item -LiteralPath $env:FMMBS_FROM -Destination $tmp -Force
    } else {
        [Net.ServicePointManager]::SecurityProtocol = [Net.ServicePointManager]::SecurityProtocol -bor [Net.SecurityProtocolType]::Tls12
        Write-Host "Downloading $Url"
        Invoke-WebRequest -Uri $Url -OutFile $tmp -UseBasicParsing
    }
    if ((Get-Item -LiteralPath $tmp).Length -lt 10000) {
        Remove-Item -LiteralPath $tmp -Force
        throw 'Downloaded file is too small - is there a published release?'
    }
    return $tmp
}

function New-StartMenuShortcut {
    $shell = New-Object -ComObject WScript.Shell
    $lnk = $shell.CreateShortcut($Shortcut)
    $lnk.TargetPath = $Exe
    $lnk.WorkingDirectory = $Dir
    $lnk.Description = 'Shake the mouse to find the cursor'
    $lnk.Save()
}

New-Item -ItemType Directory -Force -Path $Dir | Out-Null
$Downloaded = Get-App $Exe      # download first: a failed download keeps the old copy running
Stop-Running
Move-Item -LiteralPath $Downloaded -Destination $Exe -Force
Unblock-File -LiteralPath $Exe
New-StartMenuShortcut
Set-ItemProperty -Path $RunKey -Name $Name -Value "`"$Exe`""
Start-Process -FilePath $Exe

Write-Host ''
Write-Host 'Installed! Shake the mouse quickly left and right to enlarge the cursor.' -ForegroundColor Green
Write-Host 'Right-click the arrow icon in the system tray for settings.'
Write-Host "Location: $Exe"
