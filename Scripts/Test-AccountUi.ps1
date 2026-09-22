param([string]$Player = '', [switch]$Capture)
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
if (!$Player) { $Player = Join-Path $repo 'Client\Builds\Windows\MIMIC.exe' }
if (!(Test-Path -LiteralPath $Player)) { throw 'Build the Unity player first.' }
& "$PSScriptRoot\Start-Local.ps1" -NoBuild
$clients = @()
try {
    foreach ($name in @('Alice','Bob')) {
        $log = Join-Path $repo "artifacts\ui-$name.log"
        $arguments = @('-batchmode','-mimic-ui-smoke',$name,'-logFile',"`"$log`"",'-screen-width','1600','-screen-height','900','-screen-fullscreen','0')
        if ($Capture) { $arguments += @('-mimic-capture-dir', "`"$repo\artifacts\screenshots`"") }
        else { $arguments += '-nographics' }
        $clients += Start-Process -FilePath $Player -ArgumentList $arguments -WindowStyle Hidden -PassThru
    }
    $deadline = (Get-Date).AddSeconds(110)
    while (($clients | Where-Object { !$_.HasExited }).Count -gt 0) {
        if ((Get-Date) -gt $deadline) { throw 'Account UI test timed out.' }
        Start-Sleep -Milliseconds 200
    }
    foreach ($client in $clients) { if ($client.ExitCode -ne 0) { throw "Account UI client failed: $($client.ExitCode). See artifacts/ui-*.log." } }
    foreach ($name in @('Alice','Bob')) {
        if (!(Select-String -LiteralPath "$repo\artifacts\ui-$name.log" -Pattern 'MIMIC_ACCOUNT_UI_PASS' -Quiet)) { throw "Missing success marker for $name." }
    }
    Write-Host 'PASS: real Unity UI signup, validation, logout, relogin, lobby and Holdem showdown.'
} finally {
    foreach ($client in $clients) { if (!$client.HasExited) { Stop-Process -Id $client.Id } }
    & "$PSScriptRoot\Stop-Local.ps1"
}
