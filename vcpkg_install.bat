@echo off
rem Required for DX9 and DX10 project.
vcpkg install dxsdk-d3dx:x86-windows
rem If you not linked vcpkg yet.
vcpkg integrate install
pause