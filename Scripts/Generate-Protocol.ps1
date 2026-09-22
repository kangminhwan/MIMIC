param([string]$Protoc = "", [string]$VcpkgRoot = "C:\vcpkg")
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
if (!$Protoc) { $Protoc = Join-Path $VcpkgRoot 'installed\x64-windows\tools\protobuf\protoc.exe' }
if (!(Test-Path -LiteralPath $Protoc)) { throw 'Provide -Protoc with a protobuf compiler path.' }
$out = Join-Path $repo 'Client\Assets\_DEV\_Scripts\Network\Generated'
New-Item -ItemType Directory -Force -Path $out | Out-Null
& $Protoc "--proto_path=$repo\ProtocolBuffer\proto" "--csharp_out=$out" "$repo\ProtocolBuffer\proto\mimic.proto"
if ($LASTEXITCODE -ne 0) { throw 'Protocol generation failed.' }
