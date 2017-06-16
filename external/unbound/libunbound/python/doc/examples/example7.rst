.. _example_idna:

Internationalized domain name support
=====================================

Unlike the libUnbound, pyUnbound is able to handle IDN queries.

Automatic IDN DNAME conversion
-------------------------------

If we use unicode string in :meth:`unbound.ub_ctx.resolve` method,
the IDN DNAME conversion (if it is necessary) is performed on background.

Source code
...........

.. literalinclude:: example7-1.py
    :language: python

IDN converted attributes
------------------------

The :class:`unbound.ub_data` class contains attributes suffix which converts
the dname to UTF string. These attributes have the ``_idn`` suffix.

Apart from this aproach, two conversion functions exist
(:func:`unbound.idn2dname` and :func:`unbound.dname2idn`).

Source code
...........

.. literalinclude:: example7-2.py
    :language: python
