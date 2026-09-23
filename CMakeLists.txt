cmake_minimum_required(VERSION 3.24)
project(MIMIC LANGUAGES NONE)
if(NOT WIN32)
  message(FATAL_ERROR "The native TableServer requires Windows and Visual Studio 2022.")
endif()
find_program(POWERSHELL NAMES pwsh powershell REQUIRED)
add_custom_target(Mimic.TableServer ALL
  COMMAND "${POWERSHELL}" -NoProfile -ExecutionPolicy Bypass -File "${CMAKE_SOURCE_DIR}/Scripts/Build-TableServer.ps1" -Configuration "$<IF:$<CONFIG:Release>,Release,Debug>"
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
  VERBATIM)
enable_testing()
find_program(DOTNET dotnet REQUIRED)
add_test(NAME table_server_client
  COMMAND "${DOTNET}" run --project "${CMAKE_SOURCE_DIR}/tests/TableServerClient/Mimic.TableServerClient.Tests.csproj" -c Release)
