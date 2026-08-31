@echo off
vcpkg install ^
    bgfx[core,multithreaded,tools]:x86-windows ^
    sdl3:x86-windows ^
    assimp:x86-windows ^
    tinyxml:x86-windows
pause
