.. _example_asynch:

Asynchronous lookup
===================

This example performs the name lookup in the background. 
The main program keeps running while the name is resolved. 

Source code
-----------

::

	#!/usr/bin/python
	import time
	import unbound
	
	ctx = unbound.ub_ctx()
	ctx.resolvconf("/etc/resolv.conf")
	
	def call_back(my_data,status,result):
		print "Call_back:", my_data
		if status == 0 and result.havedata:
			print "Result:", result.data.address_list
			my_data['done_flag'] = True
	
	
	my_data = {'done_flag':False,'arbitrary':"object"}
	status, async_id = ctx.resolve_async("www.seznam.cz", my_data, call_back, unbound.RR_TYPE_A, unbound.RR_CLASS_IN)
	        
	while (status == 0) and (not my_data['done_flag']):
		status = ctx.process()
		time.sleep(0.1)
	
	if (status != 0):
		print "Resolve error:", unbound.ub_strerror(status)

The :meth:`unbound.ub_ctx.resolve_async` method is able to pass on any Python
object. In this example, we used a dictionary object ``my_data``.
