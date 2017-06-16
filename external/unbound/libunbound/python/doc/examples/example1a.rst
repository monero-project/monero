.. _example_resolve_name:

Resolve a name
==============

This basic example shows how to create a context and resolve a host address
(DNS record of A type).

Source code
-----------

::

    #!/usr/bin/python
    import unbound

    ctx = unbound.ub_ctx()
    ctx.resolvconf("/etc/resolv.conf")

    status, result = ctx.resolve("www.google.com")
    if status == 0 and result.havedata:
        print "Result.data:", result.data.address_list
    elif status != 0:
        print "Resolve error:", unbound.ub_strerror(status)

In contrast with the C API, the source code is more compact while the
performance of C implementation is preserved. 
The main advantage is that you need not take care about the deallocation and
allocation of context and result structures; pyUnbound module does it
automatically for you. 

If only domain name is given, the :meth:`unbound.ub_ctx.resolve` looks for
A records in IN class.
