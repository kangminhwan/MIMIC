param([string]$VcpkgRoot = "C:\vcpkg", [string]$CMake = "cmake")
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
if (!(Get-Command $CMake -ErrorAction SilentlyContinue)) {
    $CMake = 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
}
$CMake = (Get-Command $CMake -ErrorAction Stop).Source
& "$PSScriptRoot\Generate-Protocol.ps1" -VcpkgRoot $VcpkgRoot
& dotnet build "$repo\Server\Mimic.sln" -c Release
if ($LASTEXITCODE -ne 0) { throw '.NET build failed.' }
& $CMake -S $repo -B "$repo\build" -A x64 "-DCMAKE_TOOLCHAIN_FILE=$VcpkgRoot\scripts\buildsystems\vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-windows
if ($LASTEXITCODE -ne 0) { throw 'CMake configure failed.' }
& $CMake --build "$repo\build" --config Release --parallel
if ($LASTEXITCODE -ne 0) { throw 'C++ build failed.' }
& (Join-Path (Split-Path $CMake -Parent) 'ctest.exe') --test-dir "$repo\build" -C Release --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'C++ tests failed.' }
& "$PSScriptRoot\Sync-UnityDependencies.ps1"
