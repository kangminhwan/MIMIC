param([switch]$NoBuild)
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
if (!$NoBuild) { & "$PSScriptRoot\Build.ps1" }
$artifacts = Join-Path $repo 'artifacts'
New-Item -ItemType Directory -Force $artifacts | Out-Null
foreach ($port in @(5080, 5081, 7777)) {
    if (Get-NetTCPConnection -LocalPort $port -State Listen -ErrorAction SilentlyContinue) { throw "Port $port is already in use. No services were stopped." }
}
$processes = @()
$oldEnvironment = $env:ASPNETCORE_ENVIRONMENT
$env:ASPNETCORE_ENVIRONMENT = 'Development'
try {
    $processes += Start-Process dotnet -ArgumentList "`"$repo\Server\FrontServer\bin\Release\net8.0\Mimic.FrontServer.dll`"" -WindowStyle Hidden -PassThru -RedirectStandardOutput "$artifacts\front.log" -RedirectStandardError "$artifacts\front.error.log"
    $processes += Start-Process dotnet -ArgumentList "`"$repo\Server\PlatformServer\bin\Release\net8.0\Mimic.PlatformServer.dll`"" -WindowStyle Hidden -PassThru -RedirectStandardOutput "$artifacts\platform.log" -RedirectStandardError "$artifacts\platform.error.log"
    $processes += Start-Process "$repo\build\Release\Mimic.TableServer.exe" -WindowStyle Hidden -PassThru -RedirectStandardOutput "$artifacts\table.log" -RedirectStandardError "$artifacts\table.error.log"
    $ready = $false
    for ($i=0; $i -lt 30; $i++) {
        if ($processes | Where-Object HasExited) { throw 'A service exited during startup. Inspect artifacts logs.' }
        try {
            $null = Invoke-WebRequest 'http://127.0.0.1:5080/health' -TimeoutSec 1 -UseBasicParsing
            $null = Invoke-WebRequest 'http://127.0.0.1:5081/health' -TimeoutSec 1 -UseBasicParsing
            if (Get-NetTCPConnection -LocalPort 7777 -State Listen -ErrorAction SilentlyContinue) { $ready = $true; break }
        } catch { }
        Start-Sleep -Milliseconds 200
    }
    if (!$ready) { throw 'Local services did not become ready.' }
    $processes | ForEach-Object { @{ Id = $_.Id; Started = $_.StartTime.ToUniversalTime().ToString('o'); Path = $_.Path } } | ConvertTo-Json | Set-Content "$artifacts\local-processes.json" -Encoding UTF8
    Write-Host 'MIMIC is ready. Open Client in Unity and play Assets/_Scenes/Bootstrap.unity.'
    Write-Host 'Run Scripts/Stop-Local.ps1 to stop these services.'
} catch {
    foreach ($process in $processes) { if (!$process.HasExited) { Stop-Process -Id $process.Id } }
    throw
} finally { $env:ASPNETCORE_ENVIRONMENT = $oldEnvironment }
