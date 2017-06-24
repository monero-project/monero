.. _example_examine:

DNSSEC validator
================

This example program performs DNSSEC validation of a DNS lookup.

Source code
-----------

::

    #!/usr/bin/python
    import os
    from unbound import ub_ctx,RR_TYPE_A,RR_CLASS_IN

    ctx = ub_ctx()
    ctx.resolvconf("/etc/resolv.conf")
    if (os.path.isfile("keys")):
        ctx.add_ta_file("keys") #read public keys for DNSSEC verification

    status, result = ctx.resolve("www.nic.cz", RR_TYPE_A, RR_CLASS_IN)
    if status == 0 and result.havedata:

        print "Result:", result.data.address_list

        if result.secure:
            print "Result is secure"
        elif result.bogus:
            print "Result is bogus"
        else:
            print "Result is insecure"

More detailed informations can be seen in libUnbound DNSSEC tutorial `here`_.

.. _here: http://www.unbound.net/documentation/libunbound-tutorial-6.html
