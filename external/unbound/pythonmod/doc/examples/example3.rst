Response modification
=====================

This example shows how to modify the response produced by the ``iterator`` module.

As soon as the iterator module returns the response, we :

1. invalidate the data in cache
2. modify the response *TTL*
3. rewrite the data in cache
4. return modified packet

Note that the steps 1 and 3 are neccessary only in case, the python module is the first module in the processing chain.
In other cases, the validator module guarantees updating data which are produced by iterator module.

Complete source code
--------------------

.. literalinclude:: ../../examples/resmod.py
   :language: python

Testing
-------

Run Unbound server:

``root@localhost>unbound -dv -c ./test-resmod.conf``

Issue a query for name ending with "nic.cz."

``>>>dig A @127.0.0.1 www.nic.cz``

::

	;; global options:  printcmd
	;; Got answer:
	;; ->>HEADER<<- opcode: QUERY, status: NOERROR, id: 48831
	;; flags: qr rd ra; QUERY: 1, ANSWER: 1, AUTHORITY: 3, ADDITIONAL: 5
	
	;; QUESTION SECTION:
	;www.nic.cz.			IN	A
	
	;; ANSWER SECTION:
	www.nic.cz.		10	IN	A	217.31.205.50
	
	;; AUTHORITY SECTION:
	nic.cz.			10	IN	NS	e.ns.nic.cz.
	nic.cz.			10	IN	NS	a.ns.nic.cz.
	nic.cz.			10	IN	NS	c.ns.nic.cz.
	
	;; ADDITIONAL SECTION:
	a.ns.nic.cz.		10	IN	A	217.31.205.180
	a.ns.nic.cz.		10	IN	AAAA	2001:1488:dada:176::180
	c.ns.nic.cz.		10	IN	A	195.66.241.202
	c.ns.nic.cz.		10	IN	AAAA	2a01:40:1000::2
	e.ns.nic.cz.		10	IN	A	194.146.105.38
	
	;; Query time: 166 msec
	;; SERVER: 127.0.0.1#53(127.0.0.1)
	;; WHEN: Mon Jan 02 13:39:43 2009
	;; MSG SIZE  rcvd: 199

As you can see, TTL of all the records is set to 10.
