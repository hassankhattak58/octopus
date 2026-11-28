param(
    [ValidatePattern('^COM[0-9]+$')]
    [string]$Port = 'COM3',
    [switch]$ValidateOnly,
    [switch]$GenerateOnly
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location -LiteralPath $repoRoot

function Invoke-Checked {
    param([string]$Program, [string[]]$CommandArgs)
    & $Program @CommandArgs
    if ($LASTEXITCODE -ne 0) {
        throw "$Program failed with exit code $LASTEXITCODE"
    }
}

foreach ($program in @('git', 'gh', 'python')) {
    if (-not (Get-Command $program -ErrorAction SilentlyContinue)) {
        throw "Missing required command: $program"
    }
}

function Assert-SerialPort {
    if ([System.IO.Ports.SerialPort]::GetPortNames() -notcontains $Port) {
        throw "$Port is not connected. Plug in the board's socket labeled COM and check the assigned COM number."
    }
}

$branch = (& git branch --show-current).Trim()
if ($branch -ne 'main') {
    throw 'Run this script from the main branch.'
}
$remote = (& git remote get-url origin).Trim()
if ($LASTEXITCODE -ne 0 -or $remote -notmatch '(^|[/:])hassankhattak58/octopus(\.git)?$') {
    throw 'The origin remote must be hassankhattak58/octopus.'
}

$payloadPath = Join-Path $repoRoot 'payload.txt'
if (-not (Test-Path -LiteralPath $payloadPath)) {
    throw 'Create payload.txt in the repository root first.'
}
$payloadBytes = [System.IO.File]::ReadAllBytes($payloadPath)
if ($payloadBytes.Length -lt 1 -or $payloadBytes.Length -gt 8192) {
    throw 'payload.txt must contain 1-8192 bytes.'
}
if ($payloadBytes -contains 0) {
    throw 'payload.txt must be plain text without NUL bytes.'
}
Write-Host 'Payload size checked. The firmware parser validates commands and reports line errors before execution.'
if ($ValidateOnly) { return }

$scriptPath = Join-Path $repoRoot 'scripts/notepad.txt'
[System.IO.File]::WriteAllBytes($scriptPath, $payloadBytes)
if ($GenerateOnly) {
    Write-Host 'Copied payload.txt to scripts/notepad.txt without building or flashing.'
    return
}
Assert-SerialPort

$staged = & git diff --cached --name-only
if ($LASTEXITCODE -ne 0 -or $staged) {
    throw 'The Git index already has staged changes. Commit or unstage them first.'
}
$changed = @(& git diff --name-only) + @(& git ls-files --others --exclude-standard)
if ($LASTEXITCODE -ne 0) {
    throw 'Could not inspect the Git worktree.'
}
$unrelated = @($changed | Where-Object { $_ -and $_ -notin @('payload.txt', 'scripts/notepad.txt') })
if ($unrelated.Count -gt 0) {
    throw "Unrelated worktree changes found: $($unrelated -join ', ')"
}

Invoke-Checked 'git' @('add', '--', 'payload.txt', 'scripts/notepad.txt')
& git diff --cached --quiet
$hasChanges = $LASTEXITCODE -eq 1
if ($LASTEXITCODE -gt 1) {
    throw 'Could not compare staged changes.'
}

if ($hasChanges) {
    Invoke-Checked 'git' @('commit', '-m', 'Update selected DuckyScript payload')
    $sha = (& git rev-parse HEAD).Trim()
    $knownRunIds = @()
    Invoke-Checked 'git' @('push', 'origin', 'main')
    $event = 'push'
} else {
    $sha = (& git rev-parse HEAD).Trim()
    $previousJson = & gh run list --workflow Firmware --commit $sha --event workflow_dispatch --limit 100 --json databaseId
    if ($LASTEXITCODE -ne 0) {
        throw 'Could not query existing GitHub Actions runs.'
    }
    $knownRunIds = @((ConvertFrom-Json -InputObject $previousJson) | ForEach-Object { [long]$_.databaseId })
    Invoke-Checked 'gh' @('workflow', 'run', 'firmware.yml', '--ref', 'main')
    $event = 'workflow_dispatch'
}

$run = $null
for ($attempt = 0; $attempt -lt 24; $attempt++) {
    $json = & gh run list --workflow Firmware --commit $sha --limit 10 --json databaseId,status,conclusion,event,createdAt
    if ($LASTEXITCODE -ne 0) {
        throw 'Could not query GitHub Actions runs.'
    }
    $runs = ConvertFrom-Json -InputObject $json
    $run = $runs | Where-Object { $_.event -eq $event -and [long]$_.databaseId -notin $knownRunIds } | Sort-Object createdAt -Descending | Select-Object -First 1
    if ($run) { break }
    Start-Sleep -Seconds 5
}
if (-not $run) {
    throw 'The firmware workflow did not appear on GitHub.'
}

Write-Host "Building commit $sha in GitHub Actions run $($run.databaseId)..."
Invoke-Checked 'gh' @('run', 'watch', [string]$run.databaseId, '--exit-status', '--interval', '10')

$downloadDir = Join-Path $repoRoot "build/automation/$($run.databaseId)-$([guid]::NewGuid().ToString('N'))"
New-Item -ItemType Directory -Force -Path $downloadDir | Out-Null
Invoke-Checked 'gh' @('run', 'download', [string]$run.databaseId, '--name', 'octopus-esp32s3-firmware', '--dir', $downloadDir)
$imagePath = Join-Path $downloadDir 'octopus-merged.bin'
if (-not (Test-Path -LiteralPath $imagePath)) {
    throw 'GitHub Actions did not provide octopus-merged.bin.'
}

Invoke-Checked 'python' @('-m', 'esptool', '--chip', 'esp32s3', 'image-info', $imagePath)
Assert-SerialPort
Invoke-Checked 'python' @('-m', 'esptool', '--chip', 'esp32s3', '--port', $Port, 'chip-id')
Invoke-Checked 'python' @('-m', 'esptool', '--chip', 'esp32s3', '--port', $Port, 'write-flash', '0x0', $imagePath)
Write-Host 'Firmware installed. Connect native USB, then hold GPIO4 button for 800 ms and release to run the selected script.'
