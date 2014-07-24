:: Set the following variables according to your environment
set BuildProcessorArchitecture=64
set LocationDependencyBoostRoot=D:\Development\boost_1_55_0
set LocationEnvironmentVariableSetterMsbuild=C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat

call "%LocationEnvironmentVariableSetterMsbuild%"
set LocationDependencyBoostLibrary=%LocationDependencyBoostRoot%\lib%BuildProcessorArchitecture%-msvc-%VisualStudioVersion%

cd ..\..
set LocationBuildSource=%CD%
mkdir build\win%BuildProcessorArchitecture%
cd build\win%BuildProcessorArchitecture%

cmake -G "Visual Studio %VisualStudioVersion:.0=% Win%BuildProcessorArchitecture%" -DBOOST_ROOT="%LocationDependencyBoostRoot%" -DBOOST_LIBRARYDIR="%LocationDependencyBoostLibrary%" "%LocationBuildSource%"
msbuild Project.sln /p:Configuration=Release

pause