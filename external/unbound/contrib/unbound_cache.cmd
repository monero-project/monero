@echo off
rem --------------------------------------------------------------
rem -- DNS cache save/load script
rem --
rem -- Version 1.0
rem -- By Yuri Voinov (c) 2014
rem --------------------------------------------------------------

rem Variables
set prefix="C:\Program Files (x86)"
set program_path=%prefix%\Unbound
set uc=%program_path%\unbound-control.exe
set fname="unbound_cache.dmp"

rem Check Unbound installed
if exist %uc% goto start
echo Unbound control not found. Exiting...
exit 1

:start

set arg=%1

if /I "%arg%" == "-h" goto help

if "%arg%" == "" (
echo Loading cache from %program_path%\%fname%
type %program_path%\%fname%|%uc% load_cache
goto end
)

if /I "%arg%" == "-s" (
echo Saving cache to %program_path%\%fname%
%uc% dump_cache>%program_path%\%fname%
echo ok
goto end
)

if /I "%arg%" == "-l" (
echo Loading cache from %program_path%\%fname%
type %program_path%\%fname%|%uc% load_cache
goto end
)

if /I "%arg%" == "-r" (
echo Saving cache to %program_path%\%fname%
%uc% dump_cache>%program_path%\%fname%
echo ok
echo Loading cache from %program_path%\%fname%
type %program_path%\%fname%|%uc% load_cache
goto end
)

:help
echo Usage: unbound_cache.cmd [-s] or [-l] or [-r] or [-h]
echo.
echo l - Load - default mode. Warming up Unbound DNS cache from saved file. cache-ttl must be high value.
echo s - Save - save Unbound DNS cache contents to plain file with domain names.
echo r - Reload - reloadind new cache entries and refresh existing cache
echo h - this screen.
echo Note: Run without any arguments will be in default mode.
echo       Also, unbound-control must be configured.
exit 1

:end
