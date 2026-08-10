@echo off
vcpkg install ^
    bgfx[core,multithreaded,tools]:x64-windows ^
    sdl3:x64-windows ^
    assimp:x64-windows ^
    tinyxml:x64-windows
pause
