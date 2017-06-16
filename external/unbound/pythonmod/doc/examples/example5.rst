EDNS options
============

This example shows how to interact with EDNS options.

When quering unbound with the EDNS option ``65001`` and data ``0xc001`` we
expect an answer with the same EDNS option code and data ``0xdeadbeef``.


Key parts
~~~~~~~~~

This example relies on the following functionalities:


Registering EDNS options
------------------------

By registering EDNS options we can tune unbound's behavior when encountering a
query with a known EDNS option. The two available options are:

- ``bypass_cache_stage``: If set to ``True`` unbound will not try to answer
  from cache. Instead execution is passed to the modules
- ``no_aggregation``: If set to ``True`` unbound will consider this query
  unique and will not aggregate it with similar queries

Both values default to ``False``.

.. code-block:: python

    if not register_edns_option(env, 65001, bypass_cache_stage=True,
                                no_aggregation=True):
        log_info("python: Could not register EDNS option {}".format(65001))


EDNS option lists
-----------------

EDNS option lists can be found in the :class:`module_qstate` class. There are
four available lists in total:

- :class:`module_qstate.edns_opts_front_in`: options that came from the client
  side. **Should not** be changed
- :class:`module_qstate.edns_opts_back_out`: options that will be sent to the
  server side. Can be populated by edns literate modules
- :class:`module_qstate.edns_opts_back_in`: options that came from the server
  side. **Should not** be changed
- :class:`module_qstate.edns_opts_front_out`: options that will be sent to the
  client side. Can be populated by edns literate modules

Each list element has the following members:

- ``code``: the EDNS option code;
- ``data``: the EDNS option data.


Reading an EDNS option list
...........................

The lists' contents can be accessed in python by their ``_iter`` counterpart as
an iterator:

.. code-block:: python

    if not edns_opt_list_is_empty(qstate.edns_opts_front_in):
        for o in qstate.edns_opts_front_in_iter:
            log_info("python:    Code: {}, Data: '{}'".format(o.code,
                            "".join('{:02x}'.format(x) for x in o.data)))


Writing to an EDNS option list
..............................

By appending to an EDNS option list we can add new EDNS options. The new
element is going to be allocated in :class:`module_qstate.region`. The data
**must** be represented with a python ``bytearray``:

.. code-block:: python

    b = bytearray.fromhex("deadbeef")
    if not edns_opt_list_append(qstate.edns_opts_front_out,
                           o.code, b, qstate.region):
        log_info("python: Could not append EDNS option {}".format(o.code))

We can also remove an EDNS option code from an EDNS option list.

.. code-block:: python

    if not edns_opt_list_remove(edns_opt_list, code):
        log_info("python: Option code {} was not found in the "
                 "list.".format(code))

.. note:: All occurences of the EDNS option code will be removed from the list:


Controlling other modules' cache behavior
-----------------------------------------

During the modules' operation, some modules may interact with the cache
(e.g., iterator). This behavior can be controlled by using the following
:class:`module_qstate` flags:

- :class:`module_qstate.no_cache_lookup`: Modules *operating after* this module
  will not lookup the cache for an answer
- :class:`module_qstate.no_cache_store`: Modules *operating after* this module
  will not store the response in the cache

Both values default to ``0``.

.. code-block:: python

    def operate(id, event, qstate, qdata):
        if (event == MODULE_EVENT_NEW) or (event == MODULE_EVENT_PASS):
            # Detect if edns option code 56001 is present from the client side. If
            # so turn on the flags for cache management.
            if not edns_opt_list_is_empty(qstate.edns_opts_front_in):
                log_info("python: searching for edns option code 65001 during NEW "
                        "or PASS event ")
                for o in qstate.edns_opts_front_in_iter:
                    if o.code == 65001:
                        log_info("python: found edns option code 65001")
                        # Instruct other modules to not lookup for an
                        # answer in the cache.
                        qstate.no_cache_lookup = 1
                        log_info("python: enabled no_cache_lookup")

                        # Instruct other modules to not store the answer in
                        # the cache.
                        qstate.no_cache_store = 1
                        log_info("python: enabled no_cache_store")


Testing
~~~~~~~

Run the Unbound server: ::

    root@localhost$ unbound -dv -c ./test-edns.conf

In case you use your own configuration file, don't forget to enable the Python
module::

    module-config: "validator python iterator"

and use a valid script path::

    python-script: "./examples/edns.py"

Quering with EDNS option ``65001:0xc001``:

::

    root@localhost$ dig @localhost nlnetlabs.nl +ednsopt=65001:c001

    ; <<>> DiG 9.10.3-P4-Ubuntu <<>> @localhost nlnetlabs.nl +ednsopt=65001:c001
    ; (1 server found)
    ;; global options: +cmd
    ;; Got answer:
    ;; ->>HEADER<<- opcode: QUERY, status: NOERROR, id: 33450
    ;; flags: qr rd ra ad; QUERY: 1, ANSWER: 1, AUTHORITY: 4, ADDITIONAL: 3

    ;; OPT PSEUDOSECTION:
    ; EDNS: version: 0, flags:; udp: 4096
    ; OPT=65001: de ad be ef ("....")
    ;; QUESTION SECTION:
    ;nlnetlabs.nl.                  IN      A

    ;; ANSWER SECTION:
    nlnetlabs.nl.           10200   IN      A       185.49.140.10

    ;; AUTHORITY SECTION:
    nlnetlabs.nl.           10200   IN      NS      anyns.pch.net.
    nlnetlabs.nl.           10200   IN      NS      ns.nlnetlabs.nl.
    nlnetlabs.nl.           10200   IN      NS      ns-ext1.sidn.nl.
    nlnetlabs.nl.           10200   IN      NS      sec2.authdns.ripe.net.

    ;; ADDITIONAL SECTION:
    ns.nlnetlabs.nl.        10200   IN      AAAA    2a04:b900::8:0:0:60
    ns.nlnetlabs.nl.        10200   IN      A       185.49.140.60

    ;; Query time: 10 msec
    ;; SERVER: 127.0.0.1#53(127.0.0.1)
    ;; WHEN: Mon Dec 05 14:50:56 CET 2016
    ;; MSG SIZE  rcvd: 212


Complete source code
~~~~~~~~~~~~~~~~~~~~

.. literalinclude:: ../../examples/edns.py
    :language: python
