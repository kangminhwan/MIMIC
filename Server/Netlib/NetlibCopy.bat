rem Copy
rem author	: Honguk Jung
rem since	: 2023.09.04

cd..

rem xcopy .\Netlib\*.h .\Include\Netlib\ /y /d /s

xcopy .\Bin\Netlib\*.* .\Bin\TableServer\ /y /d /s /exclude:Netlib\DllOut.txt

xcopy .\TableServer\*.ini .\Bin\TableServer\ /y /d /s

xcopy .\Lib\cpprestsdk\x64\*.* .\Bin\TableServer\ /y /d /s /exclude:Netlib\DllOut.txt

xcopy .\Bin\Netlib\*.* .\Lib\ /y /d /s /exclude:Netlib\LibOut.txt