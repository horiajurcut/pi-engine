@echo off

call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

rd /q /s "build" 2>nul

mkdir build

pushd build
cl -FC -Zi ../win32_piengine.cpp user32.lib gdi32.lib
popd
