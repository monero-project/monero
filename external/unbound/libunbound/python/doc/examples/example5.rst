.. _example_resolver_only:

==============================
Resolver only
==============================

This example program shows how to perform DNS resolution only.
Unbound contains two basic modules: resolver and validator.
In case, the validator is not necessary, the validator module can be turned off using "module-config" option.
This option contains a list of module names separated by the space char. This list determined which modules should be employed and in what order.

::

	#!/usr/bin/python
	import os
	from unbound import ub_ctx,RR_TYPE_A,RR_CLASS_IN
	
	ctx = ub_ctx()
	ctx.set_option("module-config:","iterator")
	ctx.resolvconf("/etc/resolv.conf")
	
	status, result = ctx.resolve("www.google.com", RR_TYPE_A, RR_CLASS_IN)
	if status == 0 and result.havedata:
	
	    print "Result:", result.data.address_list

.. note::
   The :meth:`unbound.ub_ctx.set_option` method must be used before the first resolution (i.e. before :meth:`unbound.ub_ctx.resolve` or :meth:`unbound.ub_ctx.resolve_async` call). 

