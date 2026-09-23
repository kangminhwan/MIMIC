param([ValidateSet('Debug','Release')][string]$Configuration = 'Debug', [string]$MSBuild = '')
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
if (!$MSBuild) {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (!(Test-Path -LiteralPath $vswhere)) { throw 'Visual Studio 2022 C++ build tools are required.' }
    $MSBuild = & $vswhere -latest -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1
}
if (!$MSBuild) { throw 'MSBuild could not be located.' }
$project = Join-Path $repo 'Server\TableServer\TableServer.vcxproj'
$solutionDir = Join-Path $repo 'Server\'
& $MSBuild $project /t:Restore /p:RestorePackagesConfig=true "/p:SolutionDir=$solutionDir" /nologo /v:minimal
if ($LASTEXITCODE -ne 0) { throw 'TableServer package restore failed.' }
& $MSBuild $project "/p:Configuration=$Configuration" /p:Platform=x64 "/p:SolutionDir=$solutionDir" /m:2 /nologo /v:minimal
if ($LASTEXITCODE -ne 0) { throw 'TableServer build failed.' }
$output = Join-Path $repo 'Server\Bin\TableServer'
New-Item -ItemType Directory -Force (Join-Path $output 'data') | Out-Null
Copy-Item -Path "$repo\Server\GameData\*.csv" -Destination (Join-Path $output 'data')
$mysql = Join-Path $repo 'Server\Lib\libmysql.dll'
if (!(Test-Path -LiteralPath $mysql)) { throw 'Server/Lib/libmysql.dll is required beside the matching import library.' }
Copy-Item -LiteralPath $mysql -Destination $output
$ssl = if ($Configuration -eq 'Debug') { 'C:\vcpkg\installed\x64-windows\debug\bin\libssl-3-x64.dll' } else { 'C:\vcpkg\installed\x64-windows\bin\libssl-3-x64.dll' }
if (!(Test-Path -LiteralPath $ssl)) { throw 'The OpenSSL 3 runtime required by libmysql is missing.' }
Copy-Item -LiteralPath $ssl -Destination $output
if (!(Test-Path -LiteralPath (Join-Path $output 'TableServer.xml'))) {
    $configSource = "$repo\Server\TableServer\TableServer.xml"
    if (!(Test-Path -LiteralPath $configSource)) {
        $configSource = "$repo\Server\TableServer\TableServer.example.xml"
        Write-Warning 'Configure the generated TableServer.xml with your local database settings before starting.'
    }
    Copy-Item -LiteralPath $configSource -Destination (Join-Path $output 'TableServer.xml')
}
Write-Host "Built $output\TableServer.exe ($Configuration)."
