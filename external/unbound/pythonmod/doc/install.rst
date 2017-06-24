Installation
============

Prerequisites
-------------

Python 2.4 or higher, SWIG 1.3 or higher, GNU make

Download
--------

You can download the source codes `here`_.
The latest release is 1.1.1, Jan 15, 2009.

.. _here: unbound-1.1.1-py.tar.gz

Compiling
---------

After downloading, you can compile the Unbound library by doing::

    > tar -xzf unbound-1.1.1-py.tar.gz
    > cd unbound-1.1.1
    > ./configure --with-pythonmodule
    > make

You need GNU make to compile sources.
SWIG and Python devel libraries to compile extension module. 

Testing
-------

If the compilation is successful, you can test the extension module by::

    > cd pythonmod
    > make sudo # or "make test" or "make suexec"

This will start unbound server with language dictionary service
(see :ref:`Tutorials`).
In order to test this service, type::

   > dig TXT @127.0.0.1 aught.en._dict_.cz

Dig should print this message (czech equivalent of aught)::

   ; <<>> DiG 9.5.0-P2 <<>> TXT @127.0.0.1 aught.en._dict_.cz
   ; (1 server found)
   ;; global options:  printcmd
   ;; Got answer:
   ;; ->>HEADER<<- opcode: QUERY, status: NOERROR, id: 30085
   ;; flags: aa rd ra; QUERY: 1, ANSWER: 1, AUTHORITY: 0, ADDITIONAL: 0

   ;; QUESTION SECTION:
   ;aught.en._dict_.cz.     IN  TXT

   ;; ANSWER SECTION:
   aught.en._dict_.cz.  300 IN  TXT "nic"

   ;; Query time: 11 msec
   ;; SERVER: 127.0.0.1#53(127.0.0.1)
   ;; WHEN: Thu Jan 10 16:45:58 2009
   ;; MSG SIZE  rcvd: 52

The ``pythonmod/examples`` directory contains simple applications written in
Python.
