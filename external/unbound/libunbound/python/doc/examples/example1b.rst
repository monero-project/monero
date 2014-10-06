.. _example_reverse_lookup:

==============================
Reverse DNS lookup
==============================

Reverse DNS lookup involves determining the hostname associated with a given IP address.
This example shows how reverse lookup can be done using unbound module.

For the reverse DNS records, the special domain in-addr.arpa is reserved. 
For example, a host name for the IP address 74.125.43.147 can be obtained by issuing a DNS query for the PTR record for address 147.43.125.74.in-addr.arpa.

::

	#!/usr/bin/python
	import unbound
	
	ctx = unbound.ub_ctx()
	ctx.resolvconf("/etc/resolv.conf")
	
	status, result = ctx.resolve(unbound.reverse("74.125.43.147") + ".in-addr.arpa.", unbound.RR_TYPE_PTR, unbound.RR_CLASS_IN)
	if status == 0 and result.havedata:
		print "Result.data:", result.data.domain_list
	elif status != 0:
		print "Resolve error:", unbound.ub_strerror(status)

In order to simplify the python code, unbound module contains function which reverses the hostname components. 
This function is defined as follows::

	def reverse(domain):
		return '.'.join([a for a in domain.split(".")][::-1])


