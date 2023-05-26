SET ROOT_DIR = C:/mingw64
SET DEV_OS=windows

rem SET TARGET_PLATFORM=x86_64-w64-mingw32
SET TARGET_PLATFORM=i686-w64-mingw32

cd "%0"\..

rem mingw32-make.exe check && mingw32-make.exe all && mingw32-make.exe docs
mingw32-make.exe all
