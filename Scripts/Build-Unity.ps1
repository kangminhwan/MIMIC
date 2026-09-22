param(
    [string]$Unity = 'C:\Program Files\Unity\Hub\Editor\6000.3.22f1\Editor\Unity.exe',
    [switch]$PrepareOnly, [switch]$GenerateScenes, [switch]$Isolated
)
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
New-Item -ItemType Directory -Force "$repo\artifacts" | Out-Null
$project = Join-Path $repo 'Client'
if ($Isolated) {
    $project = Join-Path $repo 'artifacts\client-validation'
    foreach ($directory in @('Assets','Packages','ProjectSettings')) {
        New-Item -ItemType Directory -Force "$project\$directory" | Out-Null
        & robocopy "$repo\Client\$directory" "$project\$directory" /E /NFL /NDL /NJH /NJS /NP | Out-Null
        if ($LASTEXITCODE -ge 8) { throw "Failed to copy $directory to the validation project." }
    }
}
$method = if ($PrepareOnly) { 'Mimic.Editor.MimicProjectSetup.Prepare' } elseif ($GenerateScenes) { 'Mimic.Editor.MimicProjectSetup.GenerateAndBuild' } else { 'Mimic.Editor.MimicProjectSetup.BuildWindows' }
$arguments = @('-batchmode','-nographics','-quit','-projectPath',"`"$project`"",'-executeMethod',$method,'-logFile',"`"$repo\artifacts\unity-build.log`"")
$process = Start-Process -FilePath $Unity -ArgumentList $arguments -WindowStyle Hidden -PassThru
$process.WaitForExit()
if ($process.ExitCode -ne 0) { throw "Unity failed with code $($process.ExitCode). See artifacts/unity-build.log." }
if ($Isolated) {
    foreach ($directory in @('Assets\_Scenes','Assets\Prefabs')) {
        if (Test-Path -LiteralPath "$project\$directory") {
            & robocopy "$project\$directory" "$repo\Client\$directory" /E /NFL /NDL /NJH /NJS /NP | Out-Null
            if ($LASTEXITCODE -ge 8) { throw "Failed to synchronize $directory." }
        }
    }
    if (Test-Path -LiteralPath "$project\Assets\Prefabs.meta") { Copy-Item -LiteralPath "$project\Assets\Prefabs.meta" -Destination "$repo\Client\Assets\Prefabs.meta" }
    foreach ($name in @('EditorBuildSettings.asset','ProjectSettings.asset')) { Copy-Item -LiteralPath "$project\ProjectSettings\$name" -Destination "$repo\Client\ProjectSettings\$name" }
    if (Test-Path -LiteralPath "$project\Packages\packages-lock.json") { Copy-Item -LiteralPath "$project\Packages\packages-lock.json" -Destination "$repo\Client\Packages\packages-lock.json" }
    if (!$PrepareOnly) {
        & robocopy "$project\Builds\Windows" "$repo\Client\Builds\Windows" /E /NFL /NDL /NJH /NJS /NP | Out-Null
        if ($LASTEXITCODE -ge 8) { throw 'Failed to synchronize the Windows player.' }
    }
}
Write-Host 'MIMIC Unity project verified.'
