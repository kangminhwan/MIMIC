$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
$config = Get-Content -LiteralPath "$repo\Client\Assets\Resources\Config\client.json" -Raw | ConvertFrom-Json
if ($config.nativeTableServer) {
    throw 'This legacy guest-login smoke targets the retired demo server. Use Test-Smoke.ps1 for native protobuf checks; live Unity play requires original TableServer accounts and backend services.'
}

$exe = Join-Path $repo 'Client\Builds\Windows\MIMIC.exe'
if (!(Test-Path -LiteralPath $exe)) { throw 'Build the Unity client first with Build-Unity.ps1.' }
& "$PSScriptRoot\Start-Local.ps1" -NoBuild
$clients = @()
try {
    foreach ($name in @('UnityAlice', 'UnityBob')) {
        $log = Join-Path $repo "artifacts\$name.log"
        $clients += Start-Process -FilePath $exe -ArgumentList @('-batchmode', '-nographics', '-mimic-smoke', $name, '-logFile', "`"$log`"") -WindowStyle Hidden -PassThru
    }
    foreach ($client in $clients) {
        if (!$client.WaitForExit(60000)) { throw 'Unity client smoke timed out.' }
        if ($client.ExitCode -ne 0) { throw "Unity client failed with exit code $($client.ExitCode)." }
    }
    foreach ($name in @('UnityAlice', 'UnityBob')) {
        if (!(Select-String -LiteralPath "$repo\artifacts\$name.log" -Pattern 'MIMIC_UNITY_SMOKE_PASS' -Quiet)) { throw "Missing smoke success for $name" }
    }
    Write-Host 'PASS: two real Unity players completed an authenticated Protobuf Holdem hand.'
} finally {
    foreach ($client in $clients) { if (!$client.HasExited) { Stop-Process -Id $client.Id } }
    & "$PSScriptRoot\Stop-Local.ps1"
}
