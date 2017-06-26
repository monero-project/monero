Inplace callbacks
=================

This example shows how to register and use inplace callback functions. These
functions are going to be called just before unbound replies back to a client.
They can perform certain actions without interrupting unbound's execution flow
(e.g. add/remove EDNS options, manipulate the reply).

Two different scenarios will be shown:

- If answering from cache and the client used EDNS option code ``65002`` we
  will answer with the same code but with data ``0xdeadbeef``;
- When answering with a SERVFAIL we also add an empty EDNS option code
  ``65003``.


Key parts
~~~~~~~~~

This example relies on the following functionalities:


Registering inplace callback functions
--------------------------------------

There are four types of inplace callback functions:

- `inplace callback reply functions`_
- `inplace callback reply_cache functions`_
- `inplace callback reply_local functions`_
- `inplace callback reply_servfail functions`_


Inplace callback reply functions
................................

Called when answering with a *resolved* query.

The callback function's prototype is the following:

.. code-block:: python

    def inplace_reply_callback(qinfo, qstate, rep, rcode, edns, opt_list_out, region):
        """Function that will be registered as an inplace callback function.
        It will be called when answering with a resolved query.
        :param qinfo: query_info struct;
        :param qstate: module qstate. It contains the available opt_lists; It
                       SHOULD NOT be altered;
        :param rep: reply_info struct;
        :param rcode: return code for the query;
        :param edns: edns_data to be sent to the client side. It SHOULD NOT be
                     altered;
        :param opt_list_out: the list with the EDNS options that will be sent as a
                             reply. It can be populated with EDNS options;
        :param region: region to allocate temporary data. Needs to be used when we
                       want to append a new option to opt_list_out.
        :return: True on success, False on failure.
        """

.. note:: The function's name is irrelevant.

We can register such function as:

.. code-block:: python

    if not register_inplace_cb_reply(inplace_reply_callback, env, id):
        log_info("python: Could not register inplace callback function.")


Inplace callback reply_cache functions
......................................

Called when answering *from cache*.

The callback function's prototype is the following:

.. code-block:: python

    def inplace_cache_callback(qinfo, qstate, rep, rcode, edns, opt_list_out, region):
        """Function that will be registered as an inplace callback function.
        It will be called when answering from the cache.
        :param qinfo: query_info struct;
        :param qstate: module qstate. None;
        :param rep: reply_info struct;
        :param rcode: return code for the query;
        :param edns: edns_data sent from the client side. The list with the EDNS
                     options is accesible through edns.opt_list. It SHOULD NOT be
                     altered;
        :param opt_list_out: the list with the EDNS options that will be sent as a
                             reply. It can be populated with EDNS options;
        :param region: region to allocate temporary data. Needs to be used when we
                       want to append a new option to opt_list_out.
        :return: True on success, False on failure.
        """

.. note:: The function's name is irrelevant.

We can register such function as:

.. code-block:: python

    if not register_inplace_cb_reply_cache(inplace_cache_callback, env, id):
        log_info("python: Could not register inplace callback function.")


Inplace callback reply_local functions
......................................

Called when answering with *local data* or a *Chaos(CH) reply*.

The callback function's prototype is the following:

.. code-block:: python

    def inplace_local_callback(qinfo, qstate, rep, rcode, edns, opt_list_out, region):
        """Function that will be registered as an inplace callback function.
        It will be called when answering from local data.
        :param qinfo: query_info struct;
        :param qstate: module qstate. None;
        :param rep: reply_info struct;
        :param rcode: return code for the query;
        :param edns: edns_data sent from the client side. The list with the
                     EDNS options is accesible through edns.opt_list. It
                     SHOULD NOT be altered;
        :param opt_list_out: the list with the EDNS options that will be sent as a
                             reply. It can be populated with EDNS options;
        :param region: region to allocate temporary data. Needs to be used when we
                       want to append a new option to opt_list_out.
        :return: True on success, False on failure.
        """

.. note:: The function's name is irrelevant.

We can register such function as:

.. code-block:: python

    if not register_inplace_cb_reply_local(inplace_local_callback, env, id):
        log_info("python: Could not register inplace callback function.")


Inplace callback reply_servfail functions
.........................................

Called when answering with *SERVFAIL*.

The callback function's prototype is the following:

.. code-block:: python

    def inplace_servfail_callback(qinfo, qstate, rep, rcode, edns, opt_list_out, region):
        """Function that will be registered as an inplace callback function.
        It will be called when answering with SERVFAIL.
        :param qinfo: query_info struct;
        :param qstate: module qstate. If not None the relevant opt_lists are
                       available here;
        :param rep: reply_info struct. None;
        :param rcode: return code for the query. LDNS_RCODE_SERVFAIL;
        :param edns: edns_data to be sent to the client side. If qstate is None
                     edns.opt_list contains the EDNS options sent from the client
                     side. It SHOULD NOT be altered;
        :param opt_list_out: the list with the EDNS options that will be sent as a
                             reply. It can be populated with EDNS options;
        :param region: region to allocate temporary data. Needs to be used when we
                       want to append a new option to opt_list_out.
        :return: True on success, False on failure.
        """

.. note:: The function's name is irrelevant.

We can register such function as:

.. code-block:: python

    if not register_inplace_cb_reply_servfail(inplace_servfail_callback, env, id):
        log_info("python: Could not register inplace callback function.")


Testing
~~~~~~~

Run the Unbound server: ::

    root@localhost$ unbound -dv -c ./test-inplace_callbacks.conf

In case you use your own configuration file, don't forget to enable the Python
module::

    module-config: "validator python iterator"

and use a valid script path ::

    python-script: "./examples/inplace_callbacks.py"

On the first query for the nlnetlabs.nl A record we get no EDNS option back:

::

    root@localhost$ dig @localhost nlnetlabs.nl +ednsopt=65002

    ; <<>> DiG 9.10.3-P4-Ubuntu <<>> @localhost nlnetlabs.nl +ednsopt=65002
    ; (1 server found)
    ;; global options: +cmd
    ;; Got answer:
    ;; ->>HEADER<<- opcode: QUERY, status: NOERROR, id: 48057
    ;; flags: qr rd ra ad; QUERY: 1, ANSWER: 1, AUTHORITY: 4, ADDITIONAL: 3

    ;; OPT PSEUDOSECTION:
    ; EDNS: version: 0, flags:; udp: 4096
    ;; QUESTION SECTION:
    ;nlnetlabs.nl.                  IN      A

    ;; ANSWER SECTION:
    nlnetlabs.nl.           10200   IN      A       185.49.140.10

    ;; AUTHORITY SECTION:
    nlnetlabs.nl.           10200   IN      NS      ns.nlnetlabs.nl.
    nlnetlabs.nl.           10200   IN      NS      sec2.authdns.ripe.net.
    nlnetlabs.nl.           10200   IN      NS      anyns.pch.net.
    nlnetlabs.nl.           10200   IN      NS      ns-ext1.sidn.nl.

    ;; ADDITIONAL SECTION:
    ns.nlnetlabs.nl.        10200   IN      A       185.49.140.60
    ns.nlnetlabs.nl.        10200   IN      AAAA    2a04:b900::8:0:0:60

    ;; Query time: 813 msec
    ;; SERVER: 127.0.0.1#53(127.0.0.1)
    ;; WHEN: Mon Dec 05 16:15:32 CET 2016
    ;; MSG SIZE  rcvd: 204

When we issue the same query again we get a cached response and the expected
``65002: 0xdeadbeef`` EDNS option:

::

    root@localhost$ dig @localhost nlnetlabs.nl +ednsopt=65002

    ; <<>> DiG 9.10.3-P4-Ubuntu <<>> @localhost nlnetlabs.nl +ednsopt=65002
    ; (1 server found)
    ;; global options: +cmd
    ;; Got answer:
    ;; ->>HEADER<<- opcode: QUERY, status: NOERROR, id: 26489
    ;; flags: qr rd ra ad; QUERY: 1, ANSWER: 1, AUTHORITY: 4, ADDITIONAL: 3

    ;; OPT PSEUDOSECTION:
    ; EDNS: version: 0, flags:; udp: 4096
    ; OPT=65002: de ad be ef ("....")
    ;; QUESTION SECTION:
    ;nlnetlabs.nl.                  IN      A

    ;; ANSWER SECTION:
    nlnetlabs.nl.           10197   IN      A       185.49.140.10

    ;; AUTHORITY SECTION:
    nlnetlabs.nl.           10197   IN      NS      ns.nlnetlabs.nl.
    nlnetlabs.nl.           10197   IN      NS      sec2.authdns.ripe.net.
    nlnetlabs.nl.           10197   IN      NS      anyns.pch.net.
    nlnetlabs.nl.           10197   IN      NS      ns-ext1.sidn.nl.

    ;; ADDITIONAL SECTION:
    ns.nlnetlabs.nl.        10197   IN      AAAA    2a04:b900::8:0:0:60
    ns.nlnetlabs.nl.        10197   IN      A       185.49.140.60

    ;; Query time: 0 msec
    ;; SERVER: 127.0.0.1#53(127.0.0.1)
    ;; WHEN: Mon Dec 05 16:50:04 CET 2016
    ;; MSG SIZE  rcvd: 212

By issuing a query for a bogus domain unbound replies with SERVFAIL and an
empty EDNS option code ``65003``. *For this example to work unbound needs to be
validating*:

::

    root@localhost$ dig @localhost bogus.nlnetlabs.nl txt

    ; <<>> DiG 9.10.3-P4-Ubuntu <<>> @localhost bogus.nlnetlabs.nl txt
    ; (1 server found)
    ;; global options: +cmd
    ;; Got answer:
    ;; ->>HEADER<<- opcode: QUERY, status: SERVFAIL, id: 19865
    ;; flags: qr rd ra; QUERY: 1, ANSWER: 0, AUTHORITY: 0, ADDITIONAL: 1

    ;; OPT PSEUDOSECTION:
    ; EDNS: version: 0, flags:; udp: 4096
    ; OPT=65003
    ;; QUESTION SECTION:
    ;bogus.nlnetlabs.nl.            IN      TXT

    ;; Query time: 11 msec
    ;; SERVER: 127.0.0.1#53(127.0.0.1)
    ;; WHEN: Mon Dec 05 17:06:01 CET 2016
    ;; MSG SIZE  rcvd: 51


Complete source code
~~~~~~~~~~~~~~~~~~~~
.. literalinclude:: ../../examples/inplace_callbacks.py
    :language: python
