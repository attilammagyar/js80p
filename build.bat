SET PATH=C:\mingw64\bin;%PATH%
SET TARGET_PLATFORM=x86_64-w64-mingw32
SET DEV_OS=windows


cd "%0"\..

mingw32-make.exe check && mingw32-make.exe all && mingw32-make.exe docs
