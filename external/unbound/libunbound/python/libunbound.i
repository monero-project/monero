/*
 * libounbound.i: pyUnbound module (libunbound wrapper for Python)
 * 
 * Copyright (c) 2009, Zdenek Vasicek (vasicek AT fit.vutbr.cz)
 *                     Marek Vavrusa  (xvavru00 AT stud.fit.vutbr.cz)
 *
 * This software is open source.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 *    * Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 * 
 *    * Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 * 
 *    * Neither the name of the organization nor the names of its
 *      contributors may be used to endorse or promote products derived from this
 *      software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
%module unbound
%{
   #include <sys/types.h>
   #include <sys/socket.h>
   #include <netinet/in.h>
   #include <arpa/inet.h>
   #include "libunbound/unbound.h"
%}

%pythoncode %{
   import encodings.idna
   try:
       import builtins
   except ImportError:
       import __builtin__ as builtins

   # Ensure compatibility with older python versions
   if 'bytes' not in vars():
       bytes = str

   def ord(s):
       if isinstance(s, int):
           return s
       return builtins.ord(s)
%}

//%include "doc.i"
%include "file.i"

%feature("docstring") strerror "Convert error value to a human readable string."

// ================================================================================
// ub_resolve - perform resolution and validation
// ================================================================================
%typemap(in,numinputs=0,noblock=1) (struct ub_result** result)  
{ 
   struct ub_result* newubr;
   $1 = &newubr;
} 
  
/* result generation */
%typemap(argout,noblock=1) (struct ub_result** result)
{
  if(1) { /* new code block for variable on stack */
    PyObject* tuple;
    tuple = PyTuple_New(2);
    PyTuple_SetItem(tuple, 0, $result);
    if (result == 0) {
       PyTuple_SetItem(tuple, 1, SWIG_NewPointerObj(SWIG_as_voidptr(newubr), SWIGTYPE_p_ub_result, SWIG_POINTER_OWN |  0 ));
    } else {
       PyTuple_SetItem(tuple, 1, Py_None);
    }
    $result = tuple;
  }
}

                       
// ================================================================================
// ub_ctx - validation context
// ================================================================================
%nodefaultctor ub_ctx; //no default constructor & destructor
%nodefaultdtor ub_ctx;

%newobject ub_ctx_create;
%delobject ub_ctx_delete;
%rename(_ub_ctx_delete) ub_ctx_delete;

%newobject ub_resolve;

%inline %{
  void ub_ctx_free_dbg (struct ub_ctx* c) {
    printf("******** UB_CTX free 0x%lX ************\n", (long unsigned int)c);
    ub_ctx_delete(c);
  }

  //RR types
  enum enum_rr_type
  {
    /**  a host address */
    RR_TYPE_A = 1, 
    /**  an authoritative name server */
    RR_TYPE_NS = 2, 
    /**  a mail destination (Obsolete - use MX) */
    RR_TYPE_MD = 3, 
    /**  a mail forwarder (Obsolete - use MX) */
    RR_TYPE_MF = 4, 
    /**  the canonical name for an alias */
    RR_TYPE_CNAME = 5, 
    /**  marks the start of a zone of authority */
    RR_TYPE_SOA = 6, 
    /**  a mailbox domain name (EXPERIMENTAL) */
    RR_TYPE_MB = 7, 
    /**  a mail group member (EXPERIMENTAL) */
    RR_TYPE_MG = 8, 
    /**  a mail rename domain name (EXPERIMENTAL) */
    RR_TYPE_MR = 9, 
    /**  a null RR (EXPERIMENTAL) */
    RR_TYPE_NULL = 10,
    /**  a well known service description */
    RR_TYPE_WKS = 11,
    /**  a domain name pointer */
    RR_TYPE_PTR = 12,
    /**  host information */
    RR_TYPE_HINFO = 13,
    /**  mailbox or mail list information */
    RR_TYPE_MINFO = 14,
    /**  mail exchange */
    RR_TYPE_MX = 15,
    /**  text strings */
    RR_TYPE_TXT = 16,
    /**  RFC1183 */
    RR_TYPE_RP = 17,
    /**  RFC1183 */
    RR_TYPE_AFSDB = 18,
    /**  RFC1183 */
    RR_TYPE_X25 = 19,
    /**  RFC1183 */
    RR_TYPE_ISDN = 20,
    /**  RFC1183 */
    RR_TYPE_RT = 21,
    /**  RFC1706 */
    RR_TYPE_NSAP = 22,
    /**  RFC1348 */
    RR_TYPE_NSAP_PTR = 23,
    /**  2535typecode */
    RR_TYPE_SIG = 24,
    /**  2535typecode */
    RR_TYPE_KEY = 25,
    /**  RFC2163 */
    RR_TYPE_PX = 26,
    /**  RFC1712 */
    RR_TYPE_GPOS = 27,
    /**  ipv6 address */
    RR_TYPE_AAAA = 28,
    /**  LOC record  RFC1876 */
    RR_TYPE_LOC = 29,
    /**  2535typecode */
    RR_TYPE_NXT = 30,
    /**  draft-ietf-nimrod-dns-01.txt */
    RR_TYPE_EID = 31,
    /**  draft-ietf-nimrod-dns-01.txt */
    RR_TYPE_NIMLOC = 32,
    /**  SRV record RFC2782 */
    RR_TYPE_SRV = 33,
    /**  http://www.jhsoft.com/rfc/af-saa-0069.000.rtf */
    RR_TYPE_ATMA = 34,
    /**  RFC2915 */
    RR_TYPE_NAPTR = 35,
    /**  RFC2230 */
    RR_TYPE_KX = 36,
    /**  RFC2538 */
    RR_TYPE_CERT = 37,
    /**  RFC2874 */
    RR_TYPE_A6 = 38,
    /**  RFC2672 */
    RR_TYPE_DNAME = 39,
    /**  dnsind-kitchen-sink-02.txt */
    RR_TYPE_SINK = 40,
    /**  Pseudo OPT record... */
    RR_TYPE_OPT = 41,
    /**  RFC3123 */
    RR_TYPE_APL = 42,
    /**  draft-ietf-dnsext-delegation */
    RR_TYPE_DS = 43,
    /**  SSH Key Fingerprint */
    RR_TYPE_SSHFP = 44,
    /**  draft-richardson-ipseckey-rr-11.txt */
    RR_TYPE_IPSECKEY = 45,
    /**  draft-ietf-dnsext-dnssec-25 */
    RR_TYPE_RRSIG = 46,
    RR_TYPE_NSEC = 47,      
    RR_TYPE_DNSKEY = 48,
    RR_TYPE_DHCID = 49,

    RR_TYPE_NSEC3 = 50,
    RR_TYPE_NSEC3PARAMS = 51,

    RR_TYPE_UINFO = 100,
    RR_TYPE_UID = 101,
    RR_TYPE_GID = 102,
    RR_TYPE_UNSPEC = 103,

    RR_TYPE_TSIG = 250,
    RR_TYPE_IXFR = 251,
    RR_TYPE_AXFR = 252,
    /**  A request for mailbox-related records (MB, MG or MR) */
    RR_TYPE_MAILB = 253,
    /**  A request for mail agent RRs (Obsolete - see MX) */
    RR_TYPE_MAILA = 254,
    /**  any type (wildcard) */
    RR_TYPE_ANY = 255,

    /* RFC 4431, 5074, DNSSEC Lookaside Validation */
    RR_TYPE_DLV = 32769,
  };

  // RR classes
  enum enum_rr_class
  { 
    /** the Internet */
    RR_CLASS_IN = 1,
    /** Chaos class */
    RR_CLASS_CH = 3,
    /** Hesiod (Dyer 87) */
    RR_CLASS_HS = 4,
    /** None class, dynamic update */
    RR_CLASS_NONE = 254,
    /** Any class */
    RR_CLASS_ANY = 255,
  };
%} 

%feature("docstring") ub_ctx "Unbound resolving and validation context. 

The validation context is created to hold the resolver status, validation keys and a small cache (containing messages, rrsets, roundtrip times, trusted keys, lameness information).

**Usage**

>>> import unbound
>>> ctx = unbound.ub_ctx()
>>> ctx.resolvconf(\"/etc/resolv.conf\")
>>> status, result = ctx.resolve(\"www.google.com\", unbound.RR_TYPE_A, unbound.RR_CLASS_IN)
>>> if status==0 and result.havedata:
>>>    print \"Result:\",result.data.address_list
Result: ['74.125.43.147', '74.125.43.99', '74.125.43.103', '74.125.43.104']
"

%extend ub_ctx
{
 %pythoncode %{
        def __init__(self):
            """Creates a resolving and validation context.
               
               An exception is invoked if the process of creation an ub_ctx instance fails.
            """
            self.this = _unbound.ub_ctx_create()
            if not self.this:
                raise Exception("Fatal error: unbound context initialization failed")

        #__swig_destroy__ = _unbound.ub_ctx_free_dbg
        __swig_destroy__ = _unbound._ub_ctx_delete

        #UB_CTX_METHODS_#   
        def add_ta(self,ta):
            """Add a trust anchor to the given context.
               
               The trust anchor is a string, on one line, that holds a valid DNSKEY or DS RR.
               
               :param ta:
                   string, with zone-format RR on one line. [domainname] [TTL optional] [type] [class optional] [rdata contents]
               :returns: (int) 0 if OK, else error.
            """
            return _unbound.ub_ctx_add_ta(self,ta)
            #parameters: struct ub_ctx *,char *,
            #retvals: int

        def add_ta_file(self,fname):
            """Add trust anchors to the given context.
               
               Pass name of a file with DS and DNSKEY records (like from dig or drill).
               
               :param fname:
                   filename of file with keyfile with trust anchors.
               :returns: (int) 0 if OK, else error.
            """
            return _unbound.ub_ctx_add_ta_file(self,fname)
            #parameters: struct ub_ctx *,char *,
            #retvals: int

        def config(self,fname):
            """setup configuration for the given context.
               
               :param fname:
                   unbound config file (not all settings applicable). This is a power-users interface that lets you specify all sorts of options. For some specific options, such as adding trust anchors, special routines exist.
               :returns: (int) 0 if OK, else error.
            """
            return _unbound.ub_ctx_config(self,fname)
            #parameters: struct ub_ctx *,char *,
            #retvals: int

        def debuglevel(self,d):
            """Set debug verbosity for the context Output is directed to stderr.
               
               :param d:
                   debug level, 0 is off, 1 is very minimal, 2 is detailed, and 3 is lots.
               :returns: (int) 0 if OK, else error.
            """
            return _unbound.ub_ctx_debuglevel(self,d)
            #parameters: struct ub_ctx *,int,
            #retvals: int

        def debugout(self,out):
            """Set debug output (and error output) to the specified stream.
               
               Pass None to disable. Default is stderr.
               
               :param out:
                   File stream to log to.
               :returns: (int) 0 if OK, else error.

               **Usage:**

                  In order to log into file, use

                  ::

                    ctx = unbound.ub_ctx()
                    fw = fopen("debug.log")
                    ctx.debuglevel(3)
                    ctx.debugout(fw)

                  Another option is to print the debug informations to stderr output

                  ::

                    ctx = unbound.ub_ctx()
                    ctx.debuglevel(10)
                    ctx.debugout(sys.stderr) 
            """
            return _unbound.ub_ctx_debugout(self,out)
            #parameters: struct ub_ctx *,void *,
            #retvals: int

        def hosts(self,fname="/etc/hosts"):
            """Read list of hosts from the filename given.
               
               Usually "/etc/hosts". These addresses are not flagged as DNSSEC secure when queried for.
               
               :param fname:
                   file name string. If None "/etc/hosts" is used.
               :returns: (int) 0 if OK, else error.
            """
            return _unbound.ub_ctx_hosts(self,fname)
            #parameters: struct ub_ctx *,char *,
            #retvals: int

        def print_local_zones(self):
            """Print the local zones and their content (RR data) to the debug output.
               
               :returns: (int) 0 if OK, else error.
            """
            return _unbound.ub_ctx_print_local_zones(self)
            #parameters: struct ub_ctx *,
            #retvals: int

        def resolvconf(self,fname="/etc/resolv.conf"):
            """Read list of nameservers to use from the filename given.
               
               Usually "/etc/resolv.conf". Uses those nameservers as caching proxies. If they do not support DNSSEC, validation may fail.
               
               Only nameservers are picked up, the searchdomain, ndots and other settings from resolv.conf(5) are ignored.
               
               :param fname:
                   file name string. If None "/etc/resolv.conf" is used.
               :returns: (int) 0 if OK, else error.
            """
            return _unbound.ub_ctx_resolvconf(self,fname)
            #parameters: struct ub_ctx *,char *,
            #retvals: int

        def set_async(self,dothread):
            """Set a context behaviour for asynchronous action.
               
               :param dothread:
                   if True, enables threading and a call to :meth:`resolve_async` creates a thread to handle work in the background. 
                   If False, a process is forked to handle work in the background. 
                   Changes to this setting after :meth:`async` calls have been made have no effect (delete and re-create the context to change).
               :returns: (int) 0 if OK, else error.
            """
            return _unbound.ub_ctx_async(self,dothread)
            #parameters: struct ub_ctx *,int,
            #retvals: int

        def set_fwd(self,addr):
            """Set machine to forward DNS queries to, the caching resolver to use.
               
               IP4 or IP6 address. Forwards all DNS requests to that machine, which is expected to run a recursive resolver. If the  is not DNSSEC-capable, validation may fail. Can be called several times, in that case the addresses are used as backup servers.
               
               To read the list of nameservers from /etc/resolv.conf (from DHCP or so), use the call :meth:`resolvconf`.
               
               :param addr:
                   address, IP4 or IP6 in string format. If the addr is None, forwarding is disabled.
               :returns: (int) 0 if OK, else error.
            """
            return _unbound.ub_ctx_set_fwd(self,addr)
            #parameters: struct ub_ctx *,char *,
            #retvals: int

        def set_option(self,opt,val):
            """Set an option for the context.

               Changes to the options after :meth:`resolve`, :meth:`resolve_async`, :meth:`zone_add`, :meth:`zone_remove`, :meth:`data_add` or :meth:`data_remove` have no effect (you have to delete and re-create the context).
               
               :param opt:
                   option name from the unbound.conf config file format. (not all settings applicable). The name includes the trailing ':' for example set_option("logfile:", "mylog.txt"); This is a power-users interface that lets you specify all sorts of options. For some specific options, such as adding trust anchors, special routines exist.
               :param val:
                   value of the option.
               :returns: (int) 0 if OK, else error.
            """
            return _unbound.ub_ctx_set_option(self,opt,val)
            #parameters: struct ub_ctx *,char *,char *,
            #retvals: int

        def trustedkeys(self,fname):
            """Add trust anchors to the given context.
               
               Pass the name of a bind-style config file with trusted-keys{}.
               
               :param fname:
                   filename of file with bind-style config entries with trust anchors.
               :returns: (int) 0 if OK, else error.
            """
            return _unbound.ub_ctx_trustedkeys(self,fname)
            #parameters: struct ub_ctx *,char *,
            #retvals: int
        #_UB_CTX_METHODS#   
        
        def zone_print(self):
            """Print local zones using debougout"""            
            _unbound.ub_ctx_print_local_zones(self)

        def zone_add(self,zonename,zonetype):
            """Add new local zone

               :param zonename: zone domain name (e.g. myzone.)
               :param zonetype: type of the zone ("static",...) 
               :returns: (int) 0 if OK, else error. 
            """ 
            return _unbound.ub_ctx_zone_add(self,zonename, zonetype)
            #parameters: struct ub_ctx *,char*, char*
            #retvals: int

        def zone_remove(self,zonename):
            """Remove local zone
            
               If exists, removes local zone with all the RRs.

               :param zonename: zone domain name
               :returns: (int) 0 if OK, else error. 
            """ 
            return _unbound.ub_ctx_zone_remove(self,zonename)
            #parameters: struct ub_ctx *,char*
            #retvals: int

        def data_add(self,rrdata):
            """Add new local RR data

               :param rrdata: string, in zone-format on one line. [domainname] [TTL optional] [type] [class optional] [rdata contents]
               :returns: (int) 0 if OK, else error. 

               **Usage**
                  The local data ...

                  ::

                    >>> ctx = unbound.ub_ctx()
                    >>> ctx.zone_add("mydomain.net.","static")
                    0
                    >>> status = ctx.data_add("test.mydomain.net. IN A 192.168.1.1")
                    0
                    >>> status, result = ctx.resolve("test.mydomain.net")
                    >>> if status==0 and result.havedata:
                    >>>    print \"Result:\",result.data.address_list
                    Result: ['192.168.1.1']

            """ 
            return _unbound.ub_ctx_data_add(self,rrdata)
            #parameters: struct ub_ctx *,char*
            #retvals: int

        def data_remove(self,rrdata):
            """Remove local RR data

               If exists, remove resource record from local zone

               :param rrdata: string, in zone-format on one line. [domainname] [TTL optional] [type] [class optional] [rdata contents]
               :returns: (int) 0 if OK, else error. 
            """ 
            return _unbound.ub_ctx_data_remove(self,rrdata)
            #parameters: struct ub_ctx *,char*
            #retvals: int

        #UB_METHODS_#
        def cancel(self,async_id):
            """Cancel an async query in progress.
               
               Its callback will not be called.
               
               :param async_id:
                   which query to cancel.
               :returns: (int) 0 if OK, else error.
            """
            return _unbound.ub_cancel(self,async_id)
            #parameters: struct ub_ctx *,int,
            #retvals: int

        def get_fd(self):
            """Get file descriptor.
               
               Wait for it to become readable, at this point answers are returned from the asynchronous validating resolver. Then call the ub_process to continue processing. This routine works immediately after context creation, the fd does not change.
               
               :returns: (int) -1 on error, or file descriptor to use select(2) with.
            """
            return _unbound.ub_fd(self)
            #parameters: struct ub_ctx *,
            #retvals: int

        def poll(self):
            """Poll a context to see if it has any new results Do not poll in a loop, instead extract the fd below to poll for readiness, and then check, or wait using the wait routine.
               
               :returns: (int) 0 if nothing to read, or nonzero if a result is available. If nonzero, call ctx_process() to do callbacks.
            """
            return _unbound.ub_poll(self)
            #parameters: struct ub_ctx *,
            #retvals: int

        def process(self):
            """Call this routine to continue processing results from the validating resolver (when the fd becomes readable).
               
               Will perform necessary callbacks.
               
               :returns: (int) 0 if OK, else error.
            """
            return _unbound.ub_process(self)
            #parameters: struct ub_ctx *,
            #retvals: int

        def resolve(self,name,rrtype=RR_TYPE_A,rrclass=RR_CLASS_IN):
            """Perform resolution and validation of the target name. 
               
               :param name:
                   domain name in text format (a string or unicode string). IDN domain name have to be passed as a unicode string.
               :param rrtype:
                   type of RR in host order (optional argument). Default value is RR_TYPE_A (A class).
               :param rrclass:
                   class of RR in host order (optional argument). Default value is RR_CLASS_IN (for internet).
               :returns: * (int) 0 if OK, else error.
                         * (:class:`ub_result`) the result data is returned in a newly allocated result structure. May be None on return, return value is set to an error in that case (out of memory).
            """
            if isinstance(name, bytes): #probably IDN
                return _unbound.ub_resolve(self,name,rrtype,rrclass)
            else:
                return _unbound.ub_resolve(self,idn2dname(name),rrtype,rrclass)
            #parameters: struct ub_ctx *,char *,int,int,
            #retvals: int,struct ub_result **

        def resolve_async(self,name,mydata,callback,rrtype=RR_TYPE_A,rrclass=RR_CLASS_IN):
            """Perform resolution and validation of the target name.
               
               Asynchronous, after a while, the callback will be called with your data and the result. 
               If an error happens during processing, your callback will be called with error set to a nonzero value (and result==None).
               
               :param name:
                   domain name in text format (a string or unicode string). IDN domain name have to be passed as a unicode string.
               :param mydata:
                   this data is your own data (you can pass arbitrary python object or None) which are passed on to the callback function.
               :param callback:
                   call-back function which is called on completion of the resolution. 
               :param rrtype:
                   type of RR in host order (optional argument). Default value is RR_TYPE_A (A class).
               :param rrclass:
                   class of RR in host order (optional argument). Default value is RR_CLASS_IN (for internet).
               :returns: * (int) 0 if OK, else error.
                         * (int) async_id, an identifier number is returned for the query as it is in progress. It can be used to cancel the query.

               **Call-back function:**
                    The call-back function looks as the follows::
                    
                        def call_back(mydata, status, result):
                            pass

                    **Parameters:** 
                        * `mydata` - mydata object
                        * `status` - 0 when a result has been found
                        * `result` - the result structure. The result may be None, in that case err is set.

            """
            if isinstance(name, bytes): #probably IDN
                return _unbound._ub_resolve_async(self,name,rrtype,rrclass,mydata,callback)
            else:
                return _unbound._ub_resolve_async(self,idn2dname(name),rrtype,rrclass,mydata,callback)
            #parameters: struct ub_ctx *,char *,int,int,void *,ub_callback_t,
            #retvals: int, int

        def wait(self):
            """Wait for a context to finish with results.
               
               Calls  after the wait for you. After the wait, there are no more outstanding asynchronous queries.
               
               :returns: (int) 0 if OK, else error.
            """
            return _unbound.ub_wait(self)
            #parameters: struct ub_ctx *,
            #retvals: int

        #_UB_METHODS#
 %}
}


// ================================================================================
// ub_result - validation and resolution results
// ================================================================================
%nodefaultctor ub_result; //no default constructor & destructor
%nodefaultdtor ub_result;

%delobject ub_resolve_free;
%rename(_ub_resolve_free) ub_resolve_free;
 
%inline %{
  void ub_resolve_free_dbg (struct ub_result* r) {
    printf("******** UB_RESOLVE free 0x%lX ************\n", (long unsigned int)r);
    ub_resolve_free(r);
  }
%} 

%feature("docstring") ub_result "The validation and resolution results."

//ub_result.rcode
%inline %{
  enum result_enum_rcode {
    RCODE_NOERROR = 0,
    RCODE_FORMERR = 1,
    RCODE_SERVFAIL = 2,
    RCODE_NXDOMAIN = 3,
    RCODE_NOTIMPL = 4,
    RCODE_REFUSED = 5,
    RCODE_YXDOMAIN = 6,
    RCODE_YXRRSET = 7,
    RCODE_NXRRSET = 8,
    RCODE_NOTAUTH = 9,
    RCODE_NOTZONE = 10
  };
%}

%pythoncode %{
   class ub_data:
      """Class which makes the resolution results accessible"""
      def __init__(self, data):
         """Creates ub_data class
            :param data: a list of the result data in RAW format
         """
         if data == None:
            raise Exception("ub_data init: No data")
         self.data = data

      def __str__(self):
         """Represents data as string"""
         return ';'.join([' '.join(map(lambda x:"%02X" % ord(x),a)) for a in self.data])

      @staticmethod
      def dname2str(s, ofs=0, maxlen=0):
         """Parses DNAME and produces a list of labels
        
            :param ofs: where the conversion should start to parse data
            :param maxlen: maximum length (0 means parse to the end)
            :returns: list of labels (string)
         """
         if not s:
            return []

         res = []
         slen = len(s)
         if maxlen > 0:
            slen = min(slen, maxlen)

         idx = ofs
         while (idx < slen):
            complen = ord(s[idx])
            # In python 3.x `str()` converts the string to unicode which is the expected text string type
            res.append(str(s[idx+1:idx+1+complen].decode()))
            idx += complen + 1

         return res

      def as_raw_data(self):
         """Returns a list of RAW strings"""
         return self.data

      raw = property(as_raw_data, doc="Returns RAW data (a list of binary encoded strings). See :meth:`as_raw_data`")

      def as_mx_list(self):
         """Represents data as a list of MX records (query for RR_TYPE_MX)
        
            :returns: list of tuples (priority, dname)
         """
         return [(256*ord(rdf[0])+ord(rdf[1]),'.'.join([a for a in self.dname2str(rdf,2)])) for rdf in self.data]
      
      mx_list = property(as_mx_list, doc="Returns a list of tuples containing priority and domain names. See :meth:`as_mx_list`")

      def as_idn_mx_list(self):
         """Represents data as a list of MX records (query for RR_TYPE_MX)
        
            :returns: list of tuples (priority, unicode dname)
         """
         return [(256*ord(rdf[0])+ord(rdf[1]),'.'.join([encodings.idna.ToUnicode(a) for a in self.dname2str(rdf,2)])) for rdf in self.data]

      mx_list_idn = property(as_idn_mx_list, doc="Returns a list of tuples containing priority and IDN domain names. See :meth:`as_idn_mx_list`")

      def as_address_list(self):
         """Represents data as a list of IP addresses (query for RR_TYPE_PTR)
        
            :returns: list of strings
         """
         return ['.'.join(map(lambda x:str(ord(x)),a)) for a in self.data]

      address_list = property(as_address_list, doc="Returns a list of IP addresses. See :meth:`as_address_list`")

      def as_domain_list(self):
         """Represents data as a list of domain names (query for RR_TYPE_A)

            :returns: list of strings
         """
         return map(lambda x:'.'.join(self.dname2str(x)), self.data)

      domain_list = property(as_domain_list, doc="Returns a list of domain names. See :meth:`as_domain_list`")

      def as_idn_domain_list(self):
         """Represents data as a list of unicode domain names (query for RR_TYPE_A)

            :returns: list of strings
         """
         return map(lambda x: '.'.join([encodings.idna.ToUnicode(a) for a in self.dname2str(x)]), self.data)

      domain_list_idn = property(as_idn_domain_list, doc="Returns a list of IDN domain names. See :meth:`as_idn_domain_list`")
%}
        
%extend ub_result
{

  %rename(_data) data;
  
  PyObject* _ub_result_data(struct ub_result* result) {
    PyObject  *list;
     int i,cnt;
     (void)self;
     if ((result == 0) || (!result->havedata) || (result->data == 0))
        return Py_None;

     for (cnt=0,i=0;;i++,cnt++) 
         if (result->data[i] == 0)
            break;
     
     list = PyList_New(cnt);
     for (i=0;i<cnt;i++) 
         PyList_SetItem(list, i, PyBytes_FromStringAndSize(result->data[i],result->len[i]));
     
     return list;
  }

  PyObject* _packet() {
      return PyBytes_FromStringAndSize($self->answer_packet, $self->answer_len);
  }
  
 %pythoncode %{
   def __init__(self):
       raise Exception("This class can't be created directly.")

   #__swig_destroy__ = _unbound.ub_resolve_free_dbg
   __swig_destroy__ = _unbound._ub_resolve_free

   #havedata = property(_unbound.ub_result_havedata_get, _unbound.ub_result_havedata_set, "Havedata property")

   rcode2str = {RCODE_NOERROR:'no error', RCODE_FORMERR:'form error', RCODE_SERVFAIL:'serv fail', RCODE_NXDOMAIN:'nx domain', RCODE_NOTIMPL:'not implemented', RCODE_REFUSED:'refused', RCODE_YXDOMAIN:'yxdomain', RCODE_YXRRSET:'yxrrset', RCODE_NXRRSET:'nxrrset', RCODE_NOTAUTH:'not auth', RCODE_NOTZONE:'not zone'}

   def _get_rcode_str(self):
       """Returns rcode in display representation form

          :returns: string
       """
       return self.rcode2str[self.rcode]

   __swig_getmethods__["rcode_str"] = _get_rcode_str
   if _newclass:rcode_str = _swig_property(_get_rcode_str)

   def _get_raw_data(self):
       """Result data, a list of network order DNS rdata items. 

          Data are represented as a list of strings. To decode RAW data to the list of IP addresses use :attr:`data` attribute which returns an :class:`ub_data` instance containing conversion function.
       """
       return self._ub_result_data(self)

   __swig_getmethods__["rawdata"] = _get_raw_data
   rawdata = property(_get_raw_data, doc="Returns raw data, a list of rdata items. To decode RAW data use the :attr:`data` attribute which returns an instance of :class:`ub_data` containing the conversion functions.")

   def _get_data(self):
       if not self.havedata: return None
       return ub_data(self._ub_result_data(self))
  
   __swig_getmethods__["data"] = _get_data
   __swig_getmethods__["packet"] = _packet
   data = property(_get_data, doc="Returns :class:`ub_data` instance containing various decoding functions or None")

%}
             
}

%exception ub_resolve
%{ 
  //printf("resolve_start(%lX)\n",(long unsigned int)arg1);
  Py_BEGIN_ALLOW_THREADS 
  $function 
  Py_END_ALLOW_THREADS 
  //printf("resolve_stop()\n");
%} 

%include "libunbound/unbound.h"

%inline %{
  //SWIG will see the ub_ctx as a class
  struct ub_ctx {
  };
%}

//ub_ctx_debugout void* parameter correction
int ub_ctx_debugout(struct ub_ctx* ctx, FILE* out);

// ================================================================================
// ub_resolve_async - perform asynchronous resolution and validation
// ================================================================================

%typemap(in,numinputs=0,noblock=1) (int* async_id)  
{ 
   int asyncid = -1;
   $1 = &asyncid;
} 

%apply PyObject* {void* mydata}
       
/* result generation */
%typemap(argout,noblock=1) (int* async_id)
{
  if(1) { /* new code block for variable on stack */
    PyObject* tuple;
    tuple = PyTuple_New(2);
    PyTuple_SetItem(tuple, 0, $result);
    PyTuple_SetItem(tuple, 1, SWIG_From_int(asyncid));
    $result = tuple;
  }
}

// Grab a Python function object as a Python object.
%typemap(in) (PyObject *pyfunc) {
  if (!PyCallable_Check($input)) 
  {
     PyErr_SetString(PyExc_TypeError, "Need a callable object!");
     return NULL;
  }
  $1 = $input;
}
   
// Python callback workaround
int _ub_resolve_async(struct ub_ctx* ctx, char* name, int rrtype, int rrclass, void* mydata, PyObject *pyfunc, int* async_id);

%{
   struct cb_data {
      PyObject* data;
      PyObject* func;
   };

   static void PythonCallBack(void* iddata, int status, struct ub_result* result)
   {
      PyObject *arglist;
      PyObject *fresult;
      struct cb_data* id;
      id = (struct cb_data*) iddata;
      arglist = Py_BuildValue("(OiO)",id->data,status, SWIG_NewPointerObj(SWIG_as_voidptr(result), SWIGTYPE_p_ub_result, 0 |  0 ));   // Build argument list
      fresult = PyEval_CallObject(id->func,arglist);     // Call Python
      Py_DECREF(id->func);
      Py_DECREF(id->data);
      free(id);
      ub_resolve_free(result);                  //free ub_result
      //ub_resolve_free_dbg(result);                  //free ub_result
      Py_DECREF(arglist);                           // Trash arglist
      Py_XDECREF(fresult);
   }

   int _ub_resolve_async(struct ub_ctx* ctx, char* name, int rrtype, int rrclass, PyObject* mydata, PyObject *pyfunc, int* async_id) {
      int r;
      struct cb_data* id;
      id = (struct cb_data*) malloc(sizeof(struct cb_data));
      id->data = mydata;
      id->func = pyfunc;
   
      r = ub_resolve_async(ctx,name,rrtype,rrclass, (void *) id, PythonCallBack, async_id);
      Py_INCREF(mydata);
      Py_INCREF(pyfunc);
      return r;
   }

%}

%pythoncode %{
    ub_resolve_async = _unbound._ub_resolve_async

    def reverse(domain):
        """Reverse domain name
        
           Usable for reverse lookups when the IP address should be reversed
        """
        return '.'.join([a for a in domain.split(".")][::-1])

    def idn2dname(idnname):
        """Converts domain name in IDN format to canonic domain name

           :param idnname: (unicode string) IDN name
           :returns: (string) domain name
        """
        return '.'.join([encodings.idna.ToASCII(a) for a in idnname.split('.')])

    def dname2idn(name):
        """Converts canonic domain name in IDN format to unicode string

            :param name: (string) domain name
            :returns: (unicode string) domain name
        """
        return '.'.join([encodings.idna.ToUnicode(a) for a in name.split('.')])

%}

