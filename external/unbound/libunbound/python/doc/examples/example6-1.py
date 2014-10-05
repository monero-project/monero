#!/usr/bin/python
from unbound import ub_ctx,ub_strerror,RR_TYPE_A,RR_CLASS_IN

ctx = ub_ctx()
ctx.resolvconf("/etc/resolv.conf")
	
status, result = ctx.resolve("test.record.xxx", RR_TYPE_A, RR_CLASS_IN)
if status == 0 and result.havedata:
    print "Result:", result.data.address_list
else:
    print "No record found"

#define new local zone
status = ctx.zone_add("xxx.","static")
if (status != 0): print "Error zone_add:",status, ub_strerror(status)

#add RR to the zone
status = ctx.data_add("test.record.xxx. IN A 1.2.3.4")
if (status != 0): print "Error data_add:",status, ub_strerror(status)

#lookup for an A record
status, result = ctx.resolve("test.record.xxx", RR_TYPE_A, RR_CLASS_IN)
if status == 0 and result.havedata:
    print "Result:", result.data.as_address_list()
else:
    print "No record found"

