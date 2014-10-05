#!/usr/bin/python
# vim:fileencoding=utf-8
#
# IDN (Internationalized Domain Name) lookup support
#
import unbound

ctx = unbound.ub_ctx()
ctx.resolvconf("/etc/resolv.conf")

status, result = ctx.resolve(u"www.háčkyčárky.cz", unbound.RR_TYPE_A, unbound.RR_CLASS_IN)
if status == 0 and result.havedata:
    print "Result:"
    print "      raw data:", result.data
    for k in result.data.address_list:
        print "      address:%s" % k

