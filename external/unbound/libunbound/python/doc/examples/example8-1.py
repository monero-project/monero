#!/usr/bin/python
# vim:fileencoding=utf-8
#
# Lookup for MX and NS records
#
import unbound

ctx = unbound.ub_ctx()
ctx.resolvconf("/etc/resolv.conf")

status, result = ctx.resolve("nic.cz", unbound.RR_TYPE_MX, unbound.RR_CLASS_IN)
if status == 0 and result.havedata:
    print "Result:"
    print "      raw data:", result.data
    for k in result.data.mx_list:
        print "      priority:%d address:%s" % k

status, result = ctx.resolve("nic.cz", unbound.RR_TYPE_A, unbound.RR_CLASS_IN)
if status == 0 and result.havedata:
    print "Result:"
    print "      raw data:", result.data
    for k in result.data.address_list:
        print "      address:%s" % k

status, result = ctx.resolve("nic.cz", unbound.RR_TYPE_NS, unbound.RR_CLASS_IN)
if status == 0 and result.havedata:
    print "Result:"
    print "      raw data:", result.data
    for k in result.data.domain_list:
        print "      host: %s" % k

