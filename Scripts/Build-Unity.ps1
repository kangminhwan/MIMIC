param([string]$Unity = 'C:\Program Files\Unity\Hub\Editor\6000.3.22f1\Editor\Unity.exe', [switch]$PrepareOnly)
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
New-Item -ItemType Directory -Force "$repo\artifacts" | Out-Null
$method = if ($PrepareOnly) { 'Mimic.Editor.MimicProjectSetup.Prepare' } else { 'Mimic.Editor.MimicProjectSetup.BuildWindows' }
$argsList = @('-batchmode', '-nographics', '-quit', '-projectPath', "`"$repo\Client`"", '-executeMethod', $method, '-logFile', "`"$repo\artifacts\unity-build.log`"")
$process = Start-Process -FilePath $Unity -ArgumentList $argsList -WindowStyle Hidden -PassThru
$process.WaitForExit()
if ($process.ExitCode -ne 0) { throw "Unity failed with code $($process.ExitCode). See artifacts/unity-build.log." }
