#!/usr/bin/python
'''
 async-lookup.py : This example shows how to use asynchronous lookups

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
from __future__ import print_function
import unbound
import time

ctx = unbound.ub_ctx()
ctx.resolvconf("/etc/resolv.conf")

def call_back(my_data,status,result):
    print("Call_back:", sorted(my_data))
    if status == 0 and result.havedata:
        print("Result:", sorted(result.data.address_list))
        my_data['done_flag'] = True


my_data = {'done_flag':False,'arbitrary':"object"}
status, async_id = ctx.resolve_async("www.nic.cz", my_data, call_back, unbound.RR_TYPE_A, unbound.RR_CLASS_IN)
        
while (status == 0) and (not my_data['done_flag']):
    status = ctx.process()
    time.sleep(0.1)

if (status != 0):
    print("Resolve error:", unbound.ub_strerror(status))
