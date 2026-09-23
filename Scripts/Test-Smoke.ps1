$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
& dotnet run --project "$repo\tests\TableServerClient\Mimic.TableServerClient.Tests.csproj" -c Release
if ($LASTEXITCODE -ne 0) { throw 'TableServer protobuf smoke tests failed.' }
