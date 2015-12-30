Use cases (examples)
====================

Dynamic DNS Service discovery (DNS-SD_)
-------------------------------------------
Synchronized with database engine, for example *MySQL*. 

.. _DNS-SD: http://www.dns-sd.org/

Firewall control
----------------
Control firewall (e.g. enable incoming SSH connection) with DNS query signed with private key. 
So firewall can blocks every service during normal operation.

Scriptable DNS-based blacklist (DNS-BL_)
-------------------------------------------
Scripted in Python with already provided features, takes advantage of DNS reply, because
almost every mail server supports DNS based blacklisting.

.. _DNS-BL: http://www.dnsbl.org

DNS based Wake-On-Lan
---------------------
Controled by secured queries secured with private key.

Dynamic translation service
---------------------------
DNS request can be translated to virtualy any answer, that's easy to implement in client side
because of many DNS libraries available.

Examples :
 * **Dictionary** - using *IDN* for non-ascii strings transfer, ``dig TXT slovo.en._dict_.nic.cz`` returns translation of "slovo" to EN.
 * **Translation** - Extends *DNS-SD*, for example DNS to Jabber to find out people logged in.
 * **Exchange rate calculator** - ``dig TXT 1000.99.czk.eur._rates_.nic.cz`` returns the given sum (1000.99 CZK) in EURs.

Dynamic ENUM service 
--------------------
Support for redirection, synchronization, etc.
