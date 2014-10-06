# -*- coding: utf-8 -*-
'''
 calc.py: DNS-based czech-english dictionary

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
import os
cz_dict = {}
en_dict = {}

def init(id, cfg):
   log_info("pythonmod: dict init")
   f = open("examples/dict_data.txt", "r")
   try:
      for line in f:
         if line.startswith('#'):
            continue
         itm = line.split("\t", 3)
         if len(itm) < 2: 
            continue
         en,cs = itm[0:2]

         if not (cs in cz_dict):
            cz_dict[cs] = [en]     # [cs] = en
         else:
            cz_dict[cs].append(en) # [cs] = en

         if not (en in en_dict):
            en_dict[en] = [cs]     # [en] = cs
         else:
            en_dict[en].append(cs) # [en] = cs

   finally:
      f.close()
   return True

def deinit(id):
   log_info("pythonmod: dict deinit")
   return True

def operate(id, event, qstate, qdata):
    if (event == MODULE_EVENT_NEW) or (event == MODULE_EVENT_PASS):

       if qstate.qinfo.qname_str.endswith("._dict_.cz."):
        
         aword = ' '.join(qstate.qinfo.qname_list[0:-4])
         adict = qstate.qinfo.qname_list[-4]

         log_info("pythonmod: dictionary look up; word:%s dict:%s" % (aword,adict))

         words = []
         if (adict == "en") and (aword in en_dict):
            words = en_dict[aword] # EN -> CS
         if (adict == "cs") and (aword in cz_dict):
            words = cz_dict[aword] # CS -> EN

         if len(words) and ((qstate.qinfo.qtype == RR_TYPE_TXT) or (qstate.qinfo.qtype == RR_TYPE_ANY)):

            msg = DNSMessage(qstate.qinfo.qname_str, RR_TYPE_TXT, RR_CLASS_IN, PKT_RD | PKT_RA | PKT_AA)
            for w in words:
                msg.answer.append("%s 300 IN TXT \"%s\"" % (qstate.qinfo.qname_str,w.replace("\"","\\\"")))

            if not msg.set_return_msg(qstate):
               qstate.ext_state[id] = MODULE_ERROR 
               return True

            qstate.return_rcode = RCODE_NOERROR
            qstate.ext_state[id] = MODULE_FINISHED 
            return True

         else:
            qstate.return_rcode = RCODE_SERVFAIL
            qstate.ext_state[id] = MODULE_FINISHED 
            return True

       else: #Pass on the unknown query to the iterator
         qstate.ext_state[id] = MODULE_WAIT_MODULE 
         return True

    elif event == MODULE_EVENT_MODDONE: #the iterator has finished
         #we don't need modify result
         qstate.ext_state[id] = MODULE_FINISHED
         return True

    log_err("pythonmod: Unknown event")
    qstate.ext_state[id] = MODULE_ERROR
    return True

def inform_super(id, qstate, superqstate, qdata):
   return True

