$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
$recordPath = Join-Path $repo 'artifacts\local-processes.json'
if (!(Test-Path -LiteralPath $recordPath)) { Write-Host 'No recorded local services.'; return }
$records = Get-Content -LiteralPath $recordPath -Raw | ConvertFrom-Json
foreach ($record in $records) {
    $process = Get-Process -Id $record.Id -ErrorAction SilentlyContinue
    if ($process -and $process.Path -eq $record.Path -and $process.StartTime.ToUniversalTime().Ticks -eq ([DateTime]$record.Started).ToUniversalTime().Ticks) {
        Stop-Process -Id $process.Id
        $process.WaitForExit(5000) | Out-Null
    }
}
Remove-Item -LiteralPath $recordPath
