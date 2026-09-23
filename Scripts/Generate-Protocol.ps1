param([string]$Protoc = "", [string]$VcpkgRoot = "C:\vcpkg")
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
if (!$Protoc) { $Protoc = Join-Path $repo 'ProtocolBuffer\protoc\bin\protoc.exe' }
if (!(Test-Path -LiteralPath $Protoc)) { throw 'Provide the repository-compatible protoc 3.21 compiler with -Protoc.' }
$out = Join-Path $repo 'Client\Assets\_DEV\_Scripts\Network\Generated'
$nativeOut = Join-Path $out 'TableServer'
New-Item -ItemType Directory -Force -Path $nativeOut | Out-Null
& $Protoc "--proto_path=$repo\ProtocolBuffer\proto" "--csharp_out=$out" "$repo\ProtocolBuffer\proto\mimic.proto"
if ($LASTEXITCODE -ne 0) { throw 'View-model protocol generation failed.' }
$schemas = @('General.proto', 'PmNet.proto', 'Server.proto', 'Operation.proto') | ForEach-Object { Join-Path "$repo\ProtocolBuffer\protoMessages" $_ }
& $Protoc "--proto_path=$repo\ProtocolBuffer\protoMessages" "--csharp_out=$repo\ProtocolBuffer\csharp" @schemas
if ($LASTEXITCODE -ne 0) { throw 'TableServer C# protocol generation failed.' }
foreach ($name in @('General.cs', 'PmNet.cs', 'Server.cs', 'Operation.cs')) {
    Copy-Item -LiteralPath (Join-Path "$repo\ProtocolBuffer\csharp" $name) -Destination $nativeOut
}
& $Protoc "--proto_path=$repo\ProtocolBuffer\protoMessages" "--cpp_out=$repo\ProtocolBuffer\cpp" @schemas
if ($LASTEXITCODE -ne 0) { throw 'TableServer C++ protocol generation failed.' }
