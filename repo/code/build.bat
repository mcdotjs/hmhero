@echo off

mkdir ..\..\build
pushd ..\..\build
cl -Zi ..\repo\code\win32_handmade.cpp User32.lib Gdi32.lib

popd