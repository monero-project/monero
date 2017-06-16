Installation
============

Prerequisites
-------------

Python 2.4 or higher, SWIG 1.3 or higher, GNU make

Compiling
---------

After downloading, you can compile the pyUnbound library by doing::

    > tar -xzf unbound-x.x.x-py.tar.gz
    > cd unbound-x.x.x
    > ./configure --with-pyunbound
    > make

You may want to enable ``--with-pythonmodule`` as well if you want to use
python as a module in the resolver.

You need ``GNU make`` to compile sources; ``SWIG`` and ``Python devel``
libraries to compile extension module. 


Testing
-------

If the compilation is successful, you can test the python LDNS extension module
by::

    > cd contrib/python
    > make testenv
    > ./dns-lookup.py

You may want to ``make install`` in the main directory since ``make testenv``
is for debugging.  In contrib/examples you can find simple applications written
in Python using the Unbound extension.
