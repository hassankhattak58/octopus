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
$instructions = @([System.IO.File]::ReadAllLines($payloadPath, [System.Text.Encoding]::UTF8) |
    Where-Object { $_.Trim() -and $_ -notmatch '^\s*REM(?:\s|$)' })
if ($instructions.Count -lt 5 -or $instructions.Count -gt 20) {
    throw 'payload.txt needs the four Notepad launch lines followed by 1-16 text actions. REM comments are allowed.'
}
if ($instructions[0] -notmatch '^\s*GUI\s+R\s*$') {
    throw 'The first action must be GUI r.'
}
if ($instructions[1] -notmatch '^\s*DELAY\s+([0-9]{1,4})\s*$') {
    throw 'The second action must be DELAY followed by milliseconds.'
}
$delayMs = [int]$Matches[1]
if ($delayMs -lt 100 -or $delayMs -gt 2000) {
    throw 'DELAY must be between 100 and 2000 milliseconds.'
}
if ($instructions[2] -notmatch '^\s*STRING\s+notepad\.exe\s*$') {
    throw 'The third action must be STRING notepad.exe.'
}
if ($instructions[3] -notmatch '^\s*ENTER\s*$') {
    throw 'The fourth action must be ENTER.'
}
$actions = New-Object System.Collections.ArrayList
$totalTextLength = 0
$totalDelayMs = 0
$stringCount = 0
foreach ($line in $instructions[4..($instructions.Count - 1)]) {
    if ($line -match '^\s*STRING\s+(.+)$') {
        $message = $Matches[1]
        if ($message.Length -gt 80 -or $message -cnotmatch '^[A-Za-z ,!.]+$') {
            throw 'Each STRING must contain 1-80 characters: letters, spaces, comma, period, or exclamation mark.'
        }
        $totalTextLength += $message.Length
        $stringCount++
        [void]$actions.Add([pscustomobject]@{ Type = 'STRING'; Text = $message; Delay = 0 })
    } elseif ($line -match '^\s*ENTER\s*$') {
        [void]$actions.Add([pscustomobject]@{ Type = 'ENTER'; Text = ''; Delay = 0 })
    } elseif ($line -match '^\s*DELAY\s+([0-9]{1,4})\s*$') {
        $actionDelay = [int]$Matches[1]
        if ($actionDelay -lt 100 -or $actionDelay -gt 2000) {
            throw 'Each DELAY must be between 100 and 2000 milliseconds.'
        }
        $totalDelayMs += $actionDelay
        [void]$actions.Add([pscustomobject]@{ Type = 'DELAY'; Text = ''; Delay = $actionDelay })
    } else {
        throw "Unsupported Notepad action: $line"
    }
}
if ($stringCount -lt 1 -or $totalTextLength -gt 320 -or $totalDelayMs -gt 10000) {
    throw 'The demo needs at least one STRING and is limited to 320 text characters and 10 seconds of added delay.'
}
Write-Host "Validated Notepad demo: $($actions.Count) actions, $totalTextLength text characters"
if ($ValidateOnly) {
    return
}

$headerPath = Join-Path $repoRoot 'main/demo_message.h'
$headerLines = @('#pragma once', '', "#define OCTOPUS_RUN_DIALOG_DELAY_MS $delayMs", '', 'static const octopus_demo_action_t octopus_demo_actions[] = {')
foreach ($action in $actions) {
    switch ($action.Type) {
        'STRING' { $headerLines += "    { OCTOPUS_ACTION_STRING, 0, `"$($action.Text)`" }," }
        'ENTER'  { $headerLines += '    { OCTOPUS_ACTION_ENTER, 0, NULL },' }
        'DELAY'  { $headerLines += "    { OCTOPUS_ACTION_DELAY, $($action.Delay), NULL }," }
    }
}
$headerLines += '};'
$headerLines += '#define OCTOPUS_DEMO_ACTION_COUNT (sizeof(octopus_demo_actions) / sizeof(octopus_demo_actions[0]))'
$header = ($headerLines -join "`n") + "`n"
[System.IO.File]::WriteAllText($headerPath, $header, (New-Object System.Text.UTF8Encoding($false)))
if ($GenerateOnly) {
    Write-Host 'Generated main/demo_message.h without building or flashing.'
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
$unrelated = @($changed | Where-Object { $_ -and $_ -notin @('payload.txt', 'main/demo_message.h') })
if ($unrelated.Count -gt 0) {
    throw "Unrelated worktree changes found: $($unrelated -join ', ')"
}

Invoke-Checked 'git' @('add', '--', 'payload.txt', 'main/demo_message.h')
& git diff --cached --quiet
$hasChanges = $LASTEXITCODE -eq 1
if ($LASTEXITCODE -gt 1) {
    throw 'Could not compare staged changes.'
}

if ($hasChanges) {
    Invoke-Checked 'git' @('commit', '-m', 'Update Notepad demo text')
    $requestedAt = (Get-Date).ToUniversalTime().AddSeconds(-2)
    Invoke-Checked 'git' @('push', 'origin', 'main')
    $event = 'push'
} else {
    $requestedAt = (Get-Date).ToUniversalTime().AddSeconds(-2)
    Invoke-Checked 'gh' @('workflow', 'run', 'firmware.yml', '--ref', 'main')
    $event = 'workflow_dispatch'
}

$sha = (& git rev-parse HEAD).Trim()
$run = $null
for ($attempt = 0; $attempt -lt 24; $attempt++) {
    $json = & gh run list --workflow Firmware --commit $sha --limit 10 --json databaseId,status,conclusion,event,createdAt
    if ($LASTEXITCODE -ne 0) {
        throw 'Could not query GitHub Actions runs.'
    }
    $runs = ConvertFrom-Json -InputObject $json
    $run = $runs | Where-Object { $_.event -eq $event -and [datetime]$_.createdAt -ge $requestedAt } | Sort-Object createdAt -Descending | Select-Object -First 1
    if ($run) { break }
    Start-Sleep -Seconds 5
}
if (-not $run) {
    throw 'The firmware workflow did not appear on GitHub.'
}

Write-Host "Building commit $sha in GitHub Actions run $($run.databaseId)..."
Invoke-Checked 'gh' @('run', 'watch', [string]$run.databaseId, '--exit-status', '--interval', '10')

$downloadDir = Join-Path $repoRoot "build/automation/$($run.databaseId)"
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
Write-Host 'Firmware installed. Connect native USB and press the button to test the Notepad text.'
