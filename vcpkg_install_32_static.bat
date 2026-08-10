@echo off
vcpkg install ^
    bgfx[core,multithreaded,tools]:x86-windows-static ^
    sdl3:x86-windows-static ^
    assimp:x86-windows-static ^
    tinyxml:x86-windows-static
pause
