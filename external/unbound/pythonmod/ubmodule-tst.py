# -*- coding: utf-8 -*-
'''
 ubmodule-tst.py:  

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

    print "Query:", ''.join(map(lambda x:chr(max(32,ord(x))),qstate.qinfo.qname)), qstate.qinfo.qname_list, 
    print "Type:",qstate.qinfo.qtype_str,"(%d)" % qstate.qinfo.qtype,
    print "Class:",qstate.qinfo.qclass_str,"(%d)" % qstate.qinfo.qclass
    print

    # TEST:
    #   > dig @127.0.0.1 www.seznam.cz A
    #   > dig @127.0.0.1 3.76.75.77.in-addr.arpa. PTR
    #   prvni dva dotazy vrati TTL 100
    #   > dig @127.0.0.1 www.seznam.cz A
    #   > dig @127.0.0.1 3.76.75.77.in-addr.arpa. PTR
    #   dalsi dva dotazy vrati TTL 10, ktere se bude s dalsimi dotazy snizovat, nez vyprsi a znovu se zaktivuje mesh
 
    if qstate.return_msg:
        printReturnMsg(qstate)

        #qdn = '.'.join(qstate.qinfo.qname_list)
        qdn = qstate.qinfo.qname_str

        #Pokud dotaz konci na nasledujici, pozmenime TTL zpravy, ktera se posle klientovi (return_msg) i zpravy v CACHE 
        if qdn.endswith(".seznam.cz.") or qdn.endswith('.in-addr.arpa.'):
            #pokud je v cache odpoved z iteratoru, pak ji zneplatnime, jinak se moduly nazavolaji do te doby, nez vyprsi TTL
            invalidateQueryInCache(qstate, qstate.return_msg.qinfo)

            if (qstate.return_msg.rep.authoritative):
                print "X"*300

            setTTL(qstate, 10) #do cache nastavime TTL na 10
            if not storeQueryInCache(qstate, qstate.return_msg.qinfo, qstate.return_msg.rep, 0):
                qstate.ext_state[id] = MODULE_ERROR
                return False

            setTTL(qstate, 100) #odpoved klientovi prijde s TTL 100
            qstate.return_rcode = RCODE_NOERROR

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
