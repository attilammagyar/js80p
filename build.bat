SET PATH=C:\mingw64\bin;%PATH%
SET PLATFORM=x86_64-w64-mingw32
SET OS=windows


cd "%0"\..

mingw32-make.exe check && mingw32-make.exe all && mingw32-make.exe docs
