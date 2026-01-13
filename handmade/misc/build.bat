@echo off

IF NOT EXIST ..\..\build mkdir ..\..\build
pushd ..\..\build
cl -F4194304 -FC -Zi ..\handmade\code\win32_handmade.cpp user32.lib gdi32.lib
REM -DHANDMADE_INTERNAL=1 -DHANDMADE_SLOW=1 -DHANDMADE_WIN32=1 
popd	