param([ValidateSet('Debug','Release')][string]$Configuration = 'Debug', [string]$VcpkgRoot = "C:\vcpkg")
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
& "$PSScriptRoot\Generate-Protocol.ps1" -VcpkgRoot $VcpkgRoot
& "$PSScriptRoot\Build-TableServer.ps1" -Configuration $Configuration
& dotnet build "$repo\Server\Mimic.sln" -c Release --nologo
if ($LASTEXITCODE -ne 0) { throw '.NET services build failed.' }
& "$PSScriptRoot\Sync-UnityDependencies.ps1"
& dotnet run --project "$repo\tests\TableServerClient\Mimic.TableServerClient.Tests.csproj" -c Release
if ($LASTEXITCODE -ne 0) { throw 'TableServer client protocol tests failed.' }
& dotnet run --project "$repo\tests\AccountTests\Mimic.AccountTests.csproj" -c Release --no-build
if ($LASTEXITCODE -ne 0) { throw 'Account service tests failed.' }
