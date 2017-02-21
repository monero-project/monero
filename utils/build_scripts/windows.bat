:: Copyright (c) 2014-2017, The Monero Project
:: 
:: All rights reserved.
:: 
:: Redistribution and use in source and binary forms, with or without modification, are
:: permitted provided that the following conditions are met:
:: 
:: 1. Redistributions of source code must retain the above copyright notice, this list of
::    conditions and the following disclaimer.
:: 
:: 2. Redistributions in binary form must reproduce the above copyright notice, this list
::    of conditions and the following disclaimer in the documentation and/or other
::    materials provided with the distribution.
:: 
:: 3. Neither the name of the copyright holder nor the names of its contributors may be
::    used to endorse or promote products derived from this software without specific
::    prior written permission.
:: 
:: THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
:: EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
:: MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
:: THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
:: SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
:: PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
:: INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
:: STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
:: THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
