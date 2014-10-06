@echo off

rem --------------------------------------------------------------
rem -- Warm up DNS cache script by your own MRU domains
rem --
rem -- Version 1.0
rem -- By Yuri Voinov (c) 2014
rem --------------------------------------------------------------

rem Check dig installed
for /f "delims=" %%a in ('where dig') do @set dig=%%a
if /I "%dig%"=="" echo Dig not found. If installed, add path to PATH environment variable. & exit 1
echo Dig found: %dig%

echo Warming up cache by MRU domains...
rem dig -f my_domains 1>nul 2>nul
rem echo Done.

for %%a in (
mail.ru
my.mail.ru
mra.mail.ru
agent.mail.ru
news.mail.ru
icq.com
lenta.ru
gazeta.ru
peerbet.ru
www.opennet.ru
snob.ru
artlebedev.ru
mail.google.com
translate.google.com
drive.google.com
google.com
google.kz
drive.google.com
blogspot.com
farmanager.com
forum.farmanager.com
plugring.farmanager.com
symantec.com
symantecliveupdate.com
shalla.de
torstatus.blutmagie.de
torproject.org
dnscrypt.org
unbound.net
getsharex.com
skype.com
vlc.org
aimp.ru
mozilla.org
libreoffice.org
piriform.com
raidcall.com
nvidia.com
intel.com
microsoft.com
windowsupdate.com
ru.wikipedia.org
www.bbc.co.uk
tengrinews.kz
) do "%dig%" %%a 1>nul 2>nul

echo Saving cache...
unbound_cache.cmd -s
echo Done.
