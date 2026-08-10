@echo off
vcpkg install ^
    bgfx[core,multithreaded,tools]:x64-windows-static ^
    sdl3:x64-windows-static ^
    assimp:x64-windows-static ^
    tinyxml:x64-windows-static
pause
