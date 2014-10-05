Response generation
=====================

This example shows how to handle queries and generate response packet.

.. note::
   If the python module is the first module and validator module is enabled (``module-config: "python validator iterator"``),
   a return_msg security flag has to be set at least to 2. Leaving security flag untouched causes that the
   response will be refused by unbound worker as unbound will consider it as non-valid response.

Complete source code
--------------------

.. literalinclude:: ../../examples/resgen.py
   :language: python

Testing
-------

Run the unbound server:

``root@localhost>unbound -dv -c ./test-resgen.conf``

Query for a A record ending with .localdomain

``dig A test.xxx.localdomain @127.0.0.1``

Dig produces the following output::

	;; global options:  printcmd
	;; Got answer:
	;; ->>HEADER<<- opcode: QUERY, status: NOERROR, id: 48426
	;; flags: qr aa rd ra; QUERY: 1, ANSWER: 1, AUTHORITY: 0, ADDITIONAL: 0
	
	;; QUESTION SECTION:
	;test.xxx.localdomain.		IN	A
	
	;; ANSWER SECTION:
	test.xxx.localdomain.	10	IN	A	127.0.0.1
	
	;; Query time: 2 msec
	;; SERVER: 127.0.0.1#53(127.0.0.1)
	;; WHEN: Mon Jan 01 12:46:02 2009
	;; MSG SIZE  rcvd: 54

As we handle (override) in python module only queries ending with "localdomain.", the unboud can still resolve host names.
