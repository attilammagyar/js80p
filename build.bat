SET ROOT_DIR = C:/mingw64
SET DEV_OS=windows

SET TARGET_PLATFORM=x86_64-w64-mingw32
rem SET TARGET_PLATFORM=i686-w64-mingw32

cd "%0"\..

mingw32-make.exe check && mingw32-make.exe all && mingw32-make.exe docs
