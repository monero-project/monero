'''
 resip.py: This example shows how to generate authoritative response
           and how to find out the IP address of a client

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


 Usage:

   dig @127.0.0.1 -t TXT what.is.my.ip.
'''

def init(id, cfg): return True

def deinit(id): return True

def inform_super(id, qstate, superqstate, qdata): return True

def operate(id, event, qstate, qdata):
    print "Operate", event,"state:",qstate

    # Please note that if this module blocks, by moving to the validator
    # to validate or iterator to lookup or spawn a subquery to look up,
    # then, other incoming queries are queued up onto this module and
    # all of them receive the same reply.
    # You can inspect the cache.

    if (event == MODULE_EVENT_NEW) or (event == MODULE_EVENT_PASS):
        if (qstate.qinfo.qname_str.endswith("what.is.my.ip.")): #query name ends with localdomain
            #create instance of DNS message (packet) with given parameters
            msg = DNSMessage(qstate.qinfo.qname_str, RR_TYPE_TXT, RR_CLASS_IN, PKT_QR | PKT_RA | PKT_AA)
            #append RR
            if (qstate.qinfo.qtype == RR_TYPE_TXT) or (qstate.qinfo.qtype == RR_TYPE_ANY):
               rl = qstate.mesh_info.reply_list
               while (rl):
                   if rl.query_reply:
                      q = rl.query_reply
		      # The TTL of 0 is mandatory, otherwise it ends up in
		      # the cache, and is returned to other IP addresses.
                      msg.answer.append("%s 0 IN TXT \"%s %d (%s)\"" % (qstate.qinfo.qname_str, q.addr,q.port,q.family))
                   rl = rl.next

            #set qstate.return_msg 
            if not msg.set_return_msg(qstate):
                qstate.ext_state[id] = MODULE_ERROR 
                return True

            #we don't need validation, result is valid
            qstate.return_msg.rep.security = 2

            qstate.return_rcode = RCODE_NOERROR
            qstate.ext_state[id] = MODULE_FINISHED 
            return True
        else:
            #pass the query to validator
            qstate.ext_state[id] = MODULE_WAIT_MODULE 
            return True

    if event == MODULE_EVENT_MODDONE:
        log_info("pythonmod: iterator module done")
        qstate.ext_state[id] = MODULE_FINISHED 
        return True
      
    log_err("pythonmod: bad event")
    qstate.ext_state[id] = MODULE_ERROR
    return True
