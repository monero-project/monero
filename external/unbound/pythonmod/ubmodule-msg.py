# -*- coding: utf-8 -*-
'''
 ubmodule-msg.py: simple response packet logger

 Authors: Zdenek Vasicek (vasicek AT fit.vutbr.cz)
          Marek Vavrusa  (xvavru00 AT stud.fit.vutbr.cz)

 Copyright (c) 2008. All rights reserved.

 This software is open source.
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:
 
 Redistributions of source code must retain the above copyright notice,
 this list of conditions and the following disclaimer.
 
 Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
'''
import os

def init(id, cfg):
    log_info("pythonmod: init called, module id is %d port: %d script: %s" % (id, cfg.port, cfg.python_script))
    return True

def deinit(id):
    log_info("pythonmod: deinit called, module id is %d" % id)
    return True

def inform_super(id, qstate, superqstate, qdata):
    return True

def setTTL(qstate, ttl):
    """Sets return_msg TTL and all the RRs TTL"""
    if qstate.return_msg:
        qstate.return_msg.rep.ttl = ttl
        if (qstate.return_msg.rep):
            for i in range(0,qstate.return_msg.rep.rrset_count):
                d = qstate.return_msg.rep.rrsets[i].entry.data
                for j in range(0,d.count+d.rrsig_count):
                    d.rr_ttl[j] = ttl

def dataHex(data, prefix=""):
    res = ""
    for i in range(0, (len(data)+15)/16):
        res += "%s0x%02X | " % (prefix, i*16)
        d = map(lambda x:ord(x), data[i*16:i*16+17])
        for ch in d:
            res += "%02X " % ch
        for i in range(0,17-len(d)):
            res += "   "
        res += "| "
        for ch in d:
            if (ch < 32) or (ch > 127):
                res += ". "
            else:
                res += "%c " % ch
        res += "\n"
    return res

def printReturnMsg(qstate):
    print "Return MSG rep   :: flags: %04X, QDcount: %d, Security:%d, TTL=%d" % (qstate.return_msg.rep.flags, qstate.return_msg.rep.qdcount,qstate.return_msg.rep.security, qstate.return_msg.rep.ttl)
    print "           qinfo :: qname:",qstate.return_msg.qinfo.qname_list, qstate.return_msg.qinfo.qname_str, "type:",qstate.return_msg.qinfo.qtype_str, "class:",qstate.return_msg.qinfo.qclass_str
    if (qstate.return_msg.rep):
        print "RRSets:",qstate.return_msg.rep.rrset_count
        prevkey = None
        for i in range(0,qstate.return_msg.rep.rrset_count):
            r = qstate.return_msg.rep.rrsets[i]
            rk = r.rk
            print i,":",rk.dname_list, rk.dname_str, "flags: %04X" % rk.flags,
            print "type:",rk.type_str,"(%d)" % ntohs(rk.type), "class:",rk.rrset_class_str,"(%d)" % ntohs(rk.rrset_class)

            d = r.entry.data
            print "    RRDatas:",d.count+d.rrsig_count
            for j in range(0,d.count+d.rrsig_count):
                print "    ",j,":","TTL=",d.rr_ttl[j],"RR data:"
                print dataHex(d.rr_data[j],"         ")


def operate(id, event, qstate, qdata):
    log_info("pythonmod: operate called, id: %d, event:%s" % (id, strmodulevent(event)))
    #print "pythonmod: per query data", qdata

    print "Query:", ''.join(map(lambda x:chr(max(32,ord(x))),qstate.qinfo.qname)), qstate.qinfo.qname_list, qstate.qinfo.qname_str,
    print "Type:",qstate.qinfo.qtype_str,"(%d)" % qstate.qinfo.qtype,
    print "Class:",qstate.qinfo.qclass_str,"(%d)" % qstate.qinfo.qclass
    print

    #if event == MODULE_EVENT_PASS: #pokud mame "validator python iterator"
    if (event == MODULE_EVENT_NEW) and (qstate.qinfo.qname_str.endswith(".seznam.cz.")): #pokud mame "python validator iterator"
        print qstate.qinfo.qname_str

        qstate.ext_state[id] = MODULE_FINISHED 

        msg = DNSMessage(qstate.qinfo.qname_str, RR_TYPE_A, RR_CLASS_IN, PKT_QR | PKT_RA | PKT_AA) #, 300)
        #msg.authority.append("xxx.seznam.cz. 10 IN A 192.168.1.1")
        #msg.additional.append("yyy.seznam.cz. 10 IN A 1.1.1.2.")

        if qstate.qinfo.qtype == RR_TYPE_A:
            msg.answer.append("%s 10 IN A 192.168.1.1" % qstate.qinfo.qname_str)
        if (qstate.qinfo.qtype == RR_TYPE_SRV) or (qstate.qinfo.qtype == RR_TYPE_ANY):
            msg.answer.append("%s 10 IN SRV 0 0 80 neinfo.example.com." % qstate.qinfo.qname_str)
        if (qstate.qinfo.qtype == RR_TYPE_TXT) or (qstate.qinfo.qtype == RR_TYPE_ANY):
            msg.answer.append("%s 10 IN TXT path=/" % qstate.qinfo.qname_str)

        if not msg.set_return_msg(qstate):
            qstate.ext_state[id] = MODULE_ERROR 
            return True

        #qstate.return_msg.rep.security = 2 #pokud nebude nasledovat validator, je zapotrebi nastavit security, aby nebyl paket zahozen v mesh_send_reply
        printReturnMsg(qstate)

        #Authoritative result can't be stored in cache
        #if (not storeQueryInCache(qstate, qstate.return_msg.qinfo, qstate.return_msg.rep, 0)):
        #    print "Can't store in cache"
        #    qstate.ext_state[id] = MODULE_ERROR
        #    return False
        #print "Store OK"

        qstate.return_rcode = RCODE_NOERROR
        return True

    if event == MODULE_EVENT_NEW:
        qstate.ext_state[id] = MODULE_WAIT_MODULE 
        return True

    if event == MODULE_EVENT_MODDONE:
        log_info("pythonmod: previous module done")
        qstate.ext_state[id] = MODULE_FINISHED 
        return True
      
    if event == MODULE_EVENT_PASS:
        log_info("pythonmod: event_pass")
        qstate.ext_state[id] = MODULE_WAIT_MODULE 
        return True

    log_err("pythonmod: BAD event")
    qstate.ext_state[id] = MODULE_ERROR
    return True

log_info("pythonmod: script loaded.")
