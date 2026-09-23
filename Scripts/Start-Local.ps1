param([switch]$NoBuild, [ValidateSet('Debug','Release')][string]$Configuration = 'Debug')
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
if (!$NoBuild) { & "$PSScriptRoot\Build-TableServer.ps1" -Configuration $Configuration }
$output = Join-Path $repo 'Server\Bin\TableServer'
$exe = Join-Path $output 'TableServer.exe'
$config = Join-Path $output 'TableServer.xml'
if (!(Test-Path -LiteralPath $exe) -or !(Test-Path -LiteralPath $config)) { throw 'Build TableServer first.' }
[xml]$settings = Get-Content -LiteralPath $config -Raw
$port = [int]$settings.Configuration.SERVER.Default_Server_Port
if (Get-NetTCPConnection -LocalPort $port -State Listen -ErrorAction SilentlyContinue) { throw "Port $port is already in use." }
$artifacts = Join-Path $repo 'artifacts'
New-Item -ItemType Directory -Force $artifacts | Out-Null
$process = Start-Process -FilePath $exe -WorkingDirectory $output -WindowStyle Hidden -PassThru -RedirectStandardOutput "$artifacts\table.log" -RedirectStandardError "$artifacts\table.error.log"
@{ Id = $process.Id; Started = $process.StartTime.ToUniversalTime().ToString('o'); Path = $process.Path } | ConvertTo-Json | Set-Content "$artifacts\local-processes.json" -Encoding utf8
Write-Host "TableServer launched as PID $($process.Id), configured port $port."
Write-Host 'It requires the original database, Redis and configured lobby services. Check artifacts/table.log for startup status.'
Write-Host 'Set Client/Assets/Resources/Config/client.json to the matching host, port, channel and application version.'
