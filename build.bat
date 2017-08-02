@echo off

rd /q /s "build" 2>nul

mkdir build
pushd build
cl -Zi ../win32_piengine.cpp user32.lib gdi32.lib
popd
