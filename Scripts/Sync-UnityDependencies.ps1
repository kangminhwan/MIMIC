$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
$packageRoot = if ($env:NUGET_PACKAGES) { $env:NUGET_PACKAGES } else { Join-Path $env:USERPROFILE '.nuget\packages' }
$package = Join-Path $packageRoot 'google.protobuf\3.28.2'
$destination = Join-Path $repo 'Client\Assets\Plugins\Protobuf'
New-Item -ItemType Directory -Force $destination | Out-Null
Copy-Item -LiteralPath (Join-Path $package 'lib\netstandard2.0\Google.Protobuf.dll') -Destination $destination
Copy-Item -LiteralPath (Join-Path $packageRoot 'system.runtime.compilerservices.unsafe\4.5.2\lib\netstandard2.0\System.Runtime.CompilerServices.Unsafe.dll') -Destination $destination
Copy-Item -LiteralPath (Join-Path $packageRoot 'system.runtime.compilerservices.unsafe\4.5.2\LICENSE.TXT') -Destination (Join-Path $destination 'Unsafe-LICENSE.txt')
# Unity supplies System.Memory and System.Buffers; Unsafe must be included explicitly.
