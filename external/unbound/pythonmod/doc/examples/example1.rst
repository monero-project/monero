.. _log_handler:

Packet logger
=============

This example shows how to log and print details about query and response.
As soon as the ``iterator`` has finished (event is
:data:`module_event_moddone`), ``qstate.return_msg`` contains response packet
or ``None``.
This packet will be send to a client that asked for it.

Complete source code
--------------------

.. literalinclude:: ../../examples/log.py
   :language: python

Testing
-------
Run the unbound server:

``root@localhost>unbound -dv -c ./test-log.conf``

In case you use own configuration file, don't forget to enable python module:
``module-config: "validator python iterator"`` and use valid script path:
``python-script: "./examples/log.py"``.

Example of output::

   [1231790168] unbound[7941:0] info: response for <f.gtld-servers.NET. AAAA IN>
   [1231790168] unbound[7941:0] info: reply from <gtld-servers.NET.> 192.5.6.31#53
   [1231790168] unbound[7941:0] info: query response was ANSWER
   [1231790168] unbound[7941:0] info: pythonmod: operate called, id: 1, event:module_event_moddone
   ----------------------------------------------------------------------------------------------------
   Query: f.gtld-servers.NET., type: AAAA (28), class: IN (1) 
   ----------------------------------------------------------------------------------------------------
   Return    reply :: flags: 8080, QDcount: 1, Security:0, TTL=86400
             qinfo :: qname: ['f', 'gtld-servers', 'NET', ''] f.gtld-servers.NET., qtype: AAAA, qclass: IN
   Reply:
   0 : ['gtld-servers', 'NET', ''] gtld-servers.NET. flags: 0000 type: SOA (6) class: IN (1)
      0 : TTL= 86400
          0x00 | 00 3A 02 41 32 05 4E 53 54 4C 44 03 43 4F 4D 00 05 | . : . A 2 . N S T L D . C O M . . 
          0x10 | 05 6E 73 74 6C 64 0C 76 65 72 69 73 69 67 6E 2D 67 | . n s t l d . v e r i s i g n - g 
          0x20 | 67 72 73 03 43 4F 4D 00 77 74 2D 64 00 00 0E 10 00 | g r s . C O M . w t - d . . . . . 
          0x30 | 00 00 03 84 00 12 75 00 00 01 51 80                | . . . . . . u . . . Q . 

