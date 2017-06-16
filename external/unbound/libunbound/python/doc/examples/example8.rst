.. _example_mxlookup:

Lookup for MX and NS records
============================

The pyUnbound extension provides functions which are able to encode RAW RDATA
produces by unbound resolver (see :class:`unbound.ub_data`).

Source code
-----------

.. literalinclude:: example8-1.py
    :language: python

Output
------

The previous example produces the following output::

    Result:
        raw data: 00 0F 05 6D 61 69 6C 34 03 6E 69 63 02 63 7A 00;00 14 02 6D 78 05 63 7A 6E 69 63 03 6F 72 67 00;00 0A 04 6D 61 69 6C 03 6E 69 63 02 63 7A 00
        priority:15 address: mail4.nic.cz.
        priority:20 address: mx.cznic.org.
        priority:10 address: mail.nic.cz.

    Result:
        raw data: D9 1F CD 32
        address: 217.31.205.50

    Result:
        raw data: 01 61 02 6E 73 03 6E 69 63 02 63 7A 00;01 65 02 6E 73 03 6E 69 63 02 63 7A 00;01 63 02 6E 73 03 6E 69 63 02 63 7A 00
        host: a.ns.nic.cz.
        host: e.ns.nic.cz.
        host: c.ns.nic.cz.
