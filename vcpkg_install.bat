@echo off
rem Required for DX9 and DX10 project.
vcpkg install d3dx-sdk:x86-windows
rem If you not linked vcpkg yet.
vcpkg integrate install
pause