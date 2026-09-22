param([switch]$UseRunningServers)
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
if (!$UseRunningServers) { & "$PSScriptRoot\Start-Local.ps1" -NoBuild }
try {
    & dotnet run --project "$repo\tests\Smoke\Mimic.Smoke.csproj" -c Release --no-build
    if ($LASTEXITCODE -ne 0) { throw 'Integration smoke failed.' }
} finally { if (!$UseRunningServers) { & "$PSScriptRoot\Stop-Local.ps1" } }
