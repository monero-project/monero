#!/usr/bin/python
# vim:fileencoding=utf-8
#
# IDN (Internationalized Domain Name) lookup support (lookup for MX)
#
import unbound

ctx = unbound.ub_ctx()
ctx.resolvconf("/etc/resolv.conf")

status, result = ctx.resolve(u"háčkyčárky.cz", unbound.RR_TYPE_MX, unbound.RR_CLASS_IN)
if status == 0 and result.havedata:
    print "Result:"
    print "      raw data:", result.data
    for k in result.data.mx_list_idn:
        print "      priority:%d address:%s" % k
