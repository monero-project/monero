.. _example_setup_ctx:

==============================
Lookup from threads
==============================

This example shows how to use unbound module from a threaded program. 
In this example, three lookup threads are created which work in background. 
Each thread resolves different DNS record. 

::

	#!/usr/bin/python
	from unbound import ub_ctx, RR_TYPE_A, RR_CLASS_IN
	from threading import Thread
	
	ctx = ub_ctx()
	ctx.resolvconf("/etc/resolv.conf")
	
	class LookupThread(Thread):
		def __init__(self,ctx, name):
			Thread.__init__(self)
			self.ctx = ctx
			self.name = name

		def run(self):
			print "Thread lookup started:",self.name
			status, result = self.ctx.resolve(self.name, RR_TYPE_A, RR_CLASS_IN)
			if status == 0 and result.havedata:
				print "  Result:",self.name,":", result.data.address_list
	
	threads = []
	for name in ["www.fit.vutbr.cz","www.vutbr.cz","www.google.com"]:
		thread = LookupThread(ctx, name)
		thread.start()
		threads.append(thread)
	    
	for thread in threads:
		thread.join()


