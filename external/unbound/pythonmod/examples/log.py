import os
'''
 calc.py: Response packet logger

 Copyright (c) 2009, Zdenek Vasicek (vasicek AT fit.vutbr.cz)
                     Marek Vavrusa  (xvavru00 AT stud.fit.vutbr.cz)

 This software is open source.
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:
 
    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
 
    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.
 
    * Neither the name of the organization nor the names of its
      contributors may be used to endorse or promote products derived from this
      software without specific prior written permission.

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

def dataHex(data, prefix=""):
    """Converts binary string data to display representation form"""
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

def logDnsMsg(qstate):
    """Logs response"""

    r  = qstate.return_msg.rep
    q  = qstate.return_msg.qinfo

    print "-"*100
    print("Query: %s, type: %s (%d), class: %s (%d) " % (
            qstate.qinfo.qname_str, qstate.qinfo.qtype_str, qstate.qinfo.qtype,
            qstate.qinfo.qclass_str, qstate.qinfo.qclass))
    print "-"*100
    print "Return    reply :: flags: %04X, QDcount: %d, Security:%d, TTL=%d" % (r.flags, r.qdcount, r.security, r.ttl)
    print "          qinfo :: qname: %s %s, qtype: %s, qclass: %s" % (str(q.qname_list), q.qname_str, q.qtype_str, q.qclass_str)

    if (r):
        print "Reply:"
        for i in range(0, r.rrset_count):
            rr = r.rrsets[i]

            rk = rr.rk
            print i,":",rk.dname_list, rk.dname_str, "flags: %04X" % rk.flags,
            print "type:",rk.type_str,"(%d)" % ntohs(rk.type), "class:",rk.rrset_class_str,"(%d)" % ntohs(rk.rrset_class)

            d = rr.entry.data
            for j in range(0,d.count+d.rrsig_count):
                print "  ",j,":","TTL=",d.rr_ttl[j],
                if (j >= d.count): print "rrsig",
                print 
                print dataHex(d.rr_data[j],"       ")

    print "-"*100

def init(id, cfg):
   log_info("pythonmod: init called, module id is %d port: %d script: %s" % (id, cfg.port, cfg.python_script))
   return True

def deinit(id):
   log_info("pythonmod: deinit called, module id is %d" % id)
   return True

def inform_super(id, qstate, superqstate, qdata):
   return True

def operate(id, event, qstate, qdata):
   log_info("pythonmod: operate called, id: %d, event:%s" % (id, strmodulevent(event)))
  
   if (event == MODULE_EVENT_NEW) or (event == MODULE_EVENT_PASS):
      #Pass on the new event to the iterator
      qstate.ext_state[id] = MODULE_WAIT_MODULE 
      return True

   if event == MODULE_EVENT_MODDONE:
      #Iterator finished, show response (if any)

      if (qstate.return_msg):
          logDnsMsg(qstate)

      qstate.ext_state[id] = MODULE_FINISHED 
      return True

   qstate.ext_state[id] = MODULE_ERROR
   return True

