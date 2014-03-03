mkdir build

cd build

cmake "-DBoost_USE_STATIC_LIBS=TRUE" -G "Visual Studio 11 Win64" ..
cd ..
pause