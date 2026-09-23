set PROTO_FOLDER_PATH=.\
set CSHARP_OUTPUT_DIR=%PROTO_FOLDER_PATH%\..\csharp
set CPP_OUTPUT_DIR=%PROTO_FOLDER_PATH%\..\cpp

..\protoc\bin\protoc --proto_path=%PROTO_FOLDER_PATH% --cpp_out=%CPP_OUTPUT_DIR% --csharp_out=%CSHARP_OUTPUT_DIR% %PROTO_FOLDER_PATH%\General.proto
..\protoc\bin\protoc --proto_path=%PROTO_FOLDER_PATH% --cpp_out=%CPP_OUTPUT_DIR% --csharp_out=%CSHARP_OUTPUT_DIR% %PROTO_FOLDER_PATH%\PmNet.proto
..\protoc\bin\protoc --proto_path=%PROTO_FOLDER_PATH% --cpp_out=%CPP_OUTPUT_DIR% --csharp_out=%CSHARP_OUTPUT_DIR% %PROTO_FOLDER_PATH%\Server.proto
..\protoc\bin\protoc --proto_path=%PROTO_FOLDER_PATH% --cpp_out=%CPP_OUTPUT_DIR% --csharp_out=%CSHARP_OUTPUT_DIR% %PROTO_FOLDER_PATH%\Operation.proto

pause