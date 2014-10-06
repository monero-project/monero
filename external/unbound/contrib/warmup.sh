#!/bin/sh

# --------------------------------------------------------------
# -- Warm up DNS cache script by your own MRU domains
# --
# -- Version 1.0
# -- By Yuri Voinov (c) 2014
# --------------------------------------------------------------

dig=`which dig`

echo "Warming up cache by MRU domains..."
$dig -f - >/dev/null 2>&1 <<EOT
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
EOT
echo "Done."

echo "Saving cache..."
/usr/local/bin/unbound_cache.sh -s
echo "Done."

exit 0
