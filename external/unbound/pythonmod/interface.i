/*
 * interface.i: unbound python module
 */
%module unboundmodule
%{
/**
 * \file
 * This is the interface between the unbound server and a python module
 * called to perform operations on queries.
 */
   #include <sys/types.h>
   #include <sys/socket.h>
   #include <netinet/in.h>
   #include <arpa/inet.h>
   #include <stdarg.h>
   #include "config.h"
   #include "util/log.h"
   #include "util/module.h"
   #include "util/netevent.h"
   #include "util/regional.h"
   #include "util/config_file.h"
   #include "util/data/msgreply.h"
   #include "util/data/packed_rrset.h"
   #include "util/data/dname.h"
   #include "util/storage/lruhash.h"
   #include "services/cache/dns.h"
   #include "services/mesh.h"
   #include "iterator/iter_delegpt.h"
   #include "iterator/iter_hints.h"
   #include "iterator/iter_utils.h"
   #include "sldns/wire2str.h"
   #include "sldns/str2wire.h"
   #include "sldns/pkthdr.h"
%}

%include "stdint.i"  /* uint_16_t can be known type now */

%inline %{
   /* converts [len][data][len][data][0] string to a List of labels (PyBytes) */
   PyObject* GetNameAsLabelList(const char* name, int len) {
     PyObject* list;
     int cnt=0, i;

     i = 0;
     while (i < len) {
        i += name[i] + 1;
        cnt++;
     }

     list = PyList_New(cnt);
     i = 0; cnt = 0;
     while (i < len) {
        PyList_SetItem(list, cnt, PyBytes_FromStringAndSize(name + i + 1, name[i]));
        i += name[i] + 1;
        cnt++;
     }
     return list;
   }
%}

/* ************************************************************************************ *
   Structure query_info
 * ************************************************************************************ */
/* Query info */
%ignore query_info::qname;
%ignore query_info::qname_len;


struct query_info {
   %immutable;
   char* qname;
   size_t qname_len;
   uint16_t qtype;
   uint16_t qclass;
   %mutable;
};

%inline %{
   enum enum_rr_class  {
      RR_CLASS_IN = 1,
      RR_CLASS_CH = 3,
      RR_CLASS_HS = 4,
      RR_CLASS_NONE = 254,
      RR_CLASS_ANY = 255,
   };

   enum enum_rr_type {
      RR_TYPE_A = 1,
      RR_TYPE_NS = 2,
      RR_TYPE_MD = 3,
      RR_TYPE_MF = 4,
      RR_TYPE_CNAME = 5,
      RR_TYPE_SOA = 6,
      RR_TYPE_MB = 7,
      RR_TYPE_MG = 8,
      RR_TYPE_MR = 9,
      RR_TYPE_NULL = 10,
      RR_TYPE_WKS = 11,
      RR_TYPE_PTR = 12,
      RR_TYPE_HINFO = 13,
      RR_TYPE_MINFO = 14,
      RR_TYPE_MX = 15,
      RR_TYPE_TXT = 16,
      RR_TYPE_RP = 17,
      RR_TYPE_AFSDB = 18,
      RR_TYPE_X25 = 19,
      RR_TYPE_ISDN = 20,
      RR_TYPE_RT = 21,
      RR_TYPE_NSAP = 22,
      RR_TYPE_NSAP_PTR = 23,
      RR_TYPE_SIG = 24,
      RR_TYPE_KEY = 25,
      RR_TYPE_PX = 26,
      RR_TYPE_GPOS = 27,
      RR_TYPE_AAAA = 28,
      RR_TYPE_LOC = 29,
      RR_TYPE_NXT = 30,
      RR_TYPE_EID = 31,
      RR_TYPE_NIMLOC = 32,
      RR_TYPE_SRV = 33,
      RR_TYPE_ATMA = 34,
      RR_TYPE_NAPTR = 35,
      RR_TYPE_KX = 36,
      RR_TYPE_CERT = 37,
      RR_TYPE_A6 = 38,
      RR_TYPE_DNAME = 39,
      RR_TYPE_SINK = 40,
      RR_TYPE_OPT = 41,
      RR_TYPE_APL = 42,
      RR_TYPE_DS = 43,
      RR_TYPE_SSHFP = 44,
      RR_TYPE_IPSECKEY = 45,
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
      RR_TYPE_MAILB = 253,
      RR_TYPE_MAILA = 254,
      RR_TYPE_ANY = 255,
      RR_TYPE_DLV = 32769,
   };

   PyObject* _get_qname(struct query_info* q) {
      return PyBytes_FromStringAndSize((char*)q->qname, q->qname_len);
   }

   PyObject* _get_qname_components(struct query_info* q) {
      return GetNameAsLabelList((const char*)q->qname, q->qname_len);
   }
%}

%inline %{
   PyObject* dnameAsStr(const char* dname) {
       char buf[LDNS_MAX_DOMAINLEN+1];
       buf[0] = '\0';
       dname_str((uint8_t*)dname, buf);
       return PyBytes_FromString(buf);
   }
%}

%extend query_info {
   %pythoncode %{
        def _get_qtype_str(self): return sldns_wire2str_type(self.qtype)
        __swig_getmethods__["qtype_str"] = _get_qtype_str
        if _newclass:qtype_str = _swig_property(_get_qtype_str)

        def _get_qclass_str(self): return sldns_wire2str_class(self.qclass)
        __swig_getmethods__["qclass_str"] = _get_qclass_str
        if _newclass:qclass_str = _swig_property(_get_qclass_str)

        __swig_getmethods__["qname"] = _unboundmodule._get_qname
        if _newclass:qname = _swig_property(_unboundmodule._get_qname)

        __swig_getmethods__["qname_list"] = _unboundmodule._get_qname_components
        if _newclass:qname_list = _swig_property(_unboundmodule._get_qname_components)

        def _get_qname_str(self): return dnameAsStr(self.qname)
        __swig_getmethods__["qname_str"] = _get_qname_str
        if _newclass:qname_str = _swig_property(_get_qname_str)
   %}
}

/* ************************************************************************************ *
   Structure packed_rrset_key
 * ************************************************************************************ */
%ignore packed_rrset_key::dname;
%ignore packed_rrset_key::dname_len;

/* RRsets */
struct packed_rrset_key {
   %immutable;
   char*    dname;
   size_t   dname_len;
   uint32_t flags;
   uint16_t type;  /* rrset type in network format */
   uint16_t rrset_class;  /* rrset class in network format */
   %mutable;
};

/**
 * This subroutine converts values between the host and network byte order.
 * Specifically, ntohs() converts 16-bit quantities from network byte order to
 * host byte order.
 */
uint16_t ntohs(uint16_t netshort);

%inline %{
   PyObject* _get_dname(struct packed_rrset_key* k) {
      return PyBytes_FromStringAndSize((char*)k->dname, k->dname_len);
   }
   PyObject* _get_dname_components(struct packed_rrset_key* k) {
      return GetNameAsLabelList((char*)k->dname, k->dname_len);
   }
%}

%extend packed_rrset_key {
   %pythoncode %{
        def _get_type_str(self): return sldns_wire2str_type(_unboundmodule.ntohs(self.type))
        __swig_getmethods__["type_str"] = _get_type_str
        if _newclass:type_str = _swig_property(_get_type_str)

        def _get_class_str(self): return sldns_wire2str_class(_unboundmodule.ntohs(self.rrset_class))
        __swig_getmethods__["rrset_class_str"] = _get_class_str
        if _newclass:rrset_class_str = _swig_property(_get_class_str)

        __swig_getmethods__["dname"] = _unboundmodule._get_dname
        if _newclass:dname = _swig_property(_unboundmodule._get_dname)

        __swig_getmethods__["dname_list"] = _unboundmodule._get_dname_components
        if _newclass:dname_list = _swig_property(_unboundmodule._get_dname_components)

        def _get_dname_str(self): return dnameAsStr(self.dname)
        __swig_getmethods__["dname_str"] = _get_dname_str
        if _newclass:dname_str = _swig_property(_get_dname_str)
   %}
}

#if defined(SWIGWORDSIZE64)
typedef long int                rrset_id_type;
#else
typedef long long int           rrset_id_type;
#endif

struct ub_packed_rrset_key {
   struct lruhash_entry entry;
   rrset_id_type id;
   struct packed_rrset_key rk;
};

struct lruhash_entry {
  lock_rw_type lock;
  struct lruhash_entry* overflow_next;
  struct lruhash_entry* lru_next;
  struct lruhash_entry* lru_prev;
  hashvalue_type hash;
  void* key;
  struct packed_rrset_data* data;
};

%ignore packed_rrset_data::rr_len;
%ignore packed_rrset_data::rr_ttl;
%ignore packed_rrset_data::rr_data;

struct packed_rrset_data {
  /* TTL (in seconds like time()) */
  uint32_t ttl;

  /* number of rrs */
  size_t count;
  /* number of rrsigs */
  size_t rrsig_count;

  enum rrset_trust trust;
  enum sec_status security;

  /* length of every rr's rdata */
  size_t* rr_len;
  /* ttl of every rr */
  uint32_t *rr_ttl;
  /* array of pointers to every rr's rdata. The rr_data[i] rdata is stored in
   * uncompressed wireformat. */
  uint8_t** rr_data;
};

%pythoncode %{
    class RRSetData_RRLen:
        def __init__(self, obj): self.obj = obj
        def __getitem__(self, index): return _unboundmodule._get_data_rr_len(self.obj, index)
        def __len__(self): return obj.count + obj.rrsig_count
    class RRSetData_RRTTL:
        def __init__(self, obj): self.obj = obj
        def __getitem__(self, index): return _unboundmodule._get_data_rr_ttl(self.obj, index)
        def __setitem__(self, index, value): _unboundmodule._set_data_rr_ttl(self.obj, index, value)
        def __len__(self): return obj.count + obj.rrsig_count
    class RRSetData_RRData:
        def __init__(self, obj): self.obj = obj
        def __getitem__(self, index): return _unboundmodule._get_data_rr_data(self.obj, index)
        def __len__(self): return obj.count + obj.rrsig_count
%}

%inline %{
   PyObject* _get_data_rr_len(struct packed_rrset_data* d, int idx) {
     if ((d != NULL) && (idx >= 0) &&
             ((size_t)idx < (d->count+d->rrsig_count)))
        return PyInt_FromLong(d->rr_len[idx]);
     return Py_None;
   }
   void _set_data_rr_ttl(struct packed_rrset_data* d, int idx, uint32_t ttl)
   {
     if ((d != NULL) && (idx >= 0) &&
             ((size_t)idx < (d->count+d->rrsig_count)))
        d->rr_ttl[idx] = ttl;
   }
   PyObject* _get_data_rr_ttl(struct packed_rrset_data* d, int idx) {
     if ((d != NULL) && (idx >= 0) &&
             ((size_t)idx < (d->count+d->rrsig_count)))
        return PyInt_FromLong(d->rr_ttl[idx]);
     return Py_None;
   }
   PyObject* _get_data_rr_data(struct packed_rrset_data* d, int idx) {
     if ((d != NULL) && (idx >= 0) &&
             ((size_t)idx < (d->count+d->rrsig_count)))
        return PyBytes_FromStringAndSize((char*)d->rr_data[idx],
                d->rr_len[idx]);
     return Py_None;
   }
%}

%extend packed_rrset_data {
   %pythoncode %{
        def _get_data_rr_len(self): return RRSetData_RRLen(self)
        __swig_getmethods__["rr_len"] = _get_data_rr_len
        if _newclass:rr_len = _swig_property(_get_data_rr_len)
        def _get_data_rr_ttl(self): return RRSetData_RRTTL(self)
        __swig_getmethods__["rr_ttl"] =_get_data_rr_ttl
        if _newclass:rr_len = _swig_property(_get_data_rr_ttl)
        def _get_data_rr_data(self): return RRSetData_RRData(self)
        __swig_getmethods__["rr_data"] = _get_data_rr_data
        if _newclass:rr_len = _swig_property(_get_data_rr_data)
   %}
}

/* ************************************************************************************ *
   Structure reply_info
 * ************************************************************************************ */
/* Messages */
%ignore reply_info::rrsets;
%ignore reply_info::ref;

struct reply_info {
   uint16_t flags;
   uint16_t qdcount;
   uint32_t ttl;
   uint32_t prefetch_ttl;

   uint16_t authoritative;
   enum sec_status security;

   size_t an_numrrsets;
   size_t ns_numrrsets;
   size_t ar_numrrsets;
   size_t rrset_count;  /* an_numrrsets + ns_numrrsets + ar_numrrsets */

   struct ub_packed_rrset_key** rrsets;
   struct rrset_ref ref[1];  /* ? */
};

struct rrset_ref {
   struct ub_packed_rrset_key* key;
   rrset_id_type id;
};

struct dns_msg {
   struct query_info qinfo;
   struct reply_info *rep;
};

%pythoncode %{
    class ReplyInfo_RRSet:
        def __init__(self, obj): self.obj = obj
        def __getitem__(self, index): return _unboundmodule._rrset_rrsets_get(self.obj, index)
        def __len__(self): return obj.rrset_count

    class ReplyInfo_Ref:
        def __init__(self, obj): self.obj = obj
        def __getitem__(self, index): return _unboundmodule._rrset_ref_get(self.obj, index)
        def __len__(self): return obj.rrset_count
%}

%inline %{
   struct ub_packed_rrset_key* _rrset_rrsets_get(struct reply_info* r, int idx) {
     if ((r != NULL) && (idx >= 0) && ((size_t)idx < r->rrset_count))
        return r->rrsets[idx];
     return NULL;
   }

   struct rrset_ref* _rrset_ref_get(struct reply_info* r, int idx) {
     if ((r != NULL) && (idx >= 0) && ((size_t)idx < r->rrset_count)) {
/* printf("_rrset_ref_get: %lX key:%lX\n", r->ref + idx, r->ref[idx].key); */
             return &(r->ref[idx]);
/*        return &(r->ref[idx]); */
     }
/* printf("_rrset_ref_get: NULL\n"); */
     return NULL;
   }
%}

%extend reply_info {
   %pythoncode %{
        def _rrset_ref_get(self): return ReplyInfo_Ref(self)
        __swig_getmethods__["ref"] = _rrset_ref_get
        if _newclass:ref = _swig_property(_rrset_ref_get)

        def _rrset_rrsets_get(self): return ReplyInfo_RRSet(self)
        __swig_getmethods__["rrsets"] = _rrset_rrsets_get
        if _newclass:rrsets = _swig_property(_rrset_rrsets_get)
   %}
}

/* ************************************************************************************ *
   Structure mesh_state
 * ************************************************************************************ */
struct mesh_state {
   struct mesh_reply* reply_list;
};

struct mesh_reply {
   struct mesh_reply* next;
   struct comm_reply query_reply;
};

struct comm_reply {

};

%inline %{

  PyObject* _comm_reply_addr_get(struct comm_reply* reply) {
     char dest[64];
     reply_addr2str(reply, dest, 64);
     if (dest[0] == 0)
        return Py_None;
     return PyBytes_FromString(dest);
  }

  PyObject* _comm_reply_family_get(struct comm_reply* reply) {

        int af = (int)((struct sockaddr_in*) &(reply->addr))->sin_family;

        switch(af) {
           case AF_INET: return PyBytes_FromString("ip4");
           case AF_INET6: return PyBytes_FromString("ip6");
           case AF_UNIX: return PyBytes_FromString("unix");
        }

        return Py_None;
  }

  PyObject* _comm_reply_port_get(struct comm_reply* reply) {
     uint16_t port;
     port = ntohs(((struct sockaddr_in*)&(reply->addr))->sin_port);
     return PyInt_FromLong(port);
  }

%}

%extend comm_reply {
   %pythoncode %{
        def _addr_get(self): return _comm_reply_addr_get(self)
        __swig_getmethods__["addr"] = _addr_get
        if _newclass:addr = _swig_property(_addr_get)

        def _port_get(self): return _comm_reply_port_get(self)
        __swig_getmethods__["port"] = _port_get
        if _newclass:port = _swig_property(_port_get)

        def _family_get(self): return _comm_reply_family_get(self)
        __swig_getmethods__["family"] = _family_get
        if _newclass:family = _swig_property(_family_get)
   %}
}

/* ************************************************************************************ *
   Structure edns_option
 * ************************************************************************************ */
/* Rename the members to follow the python convention of marking them as
 * private. Access to the opt_code and opt_data members is given by the later
 * python defined code and data members respectively. */
%rename(_next) edns_option::next;
%rename(_opt_code) edns_option::opt_code;
%rename(_opt_len) edns_option::opt_len;
%rename(_opt_data) edns_option::opt_data;
struct edns_option {
    struct edns_option* next;
    uint16_t opt_code;
    size_t opt_len;
    uint8_t* opt_data;
};

%inline %{
    PyObject* _edns_option_opt_code_get(struct edns_option* option) {
        uint16_t opt_code = option->opt_code;
        return PyInt_FromLong(opt_code);
    }

    PyObject* _edns_option_opt_data_get(struct edns_option* option) {
        return PyByteArray_FromStringAndSize((void*)option->opt_data,
            option->opt_len);
    }
%}
%extend edns_option {
    %pythoncode %{
        def _opt_code_get(self): return _edns_option_opt_code_get(self)
        __swig_getmethods__["code"] = _opt_code_get
        if _newclass: opt_code = _swig_property(_opt_code_get)

        def _opt_data_get(self): return _edns_option_opt_data_get(self)
        __swig_getmethods__["data"] = _opt_data_get
        if _newclass: opt_data = _swig_property(_opt_data_get)
    %}
}

/* ************************************************************************************ *
   Structure edns_data
 * ************************************************************************************ */
/* This is ignored because we will pass a double pointer of this to Python
 * with custom getmethods. This is done to bypass Swig's behavior to pass NULL
 * pointers as None. */
%ignore edns_data::opt_list;
struct edns_data {
    int edns_present;
    uint8_t ext_rcode;
    uint8_t edns_version;
    uint16_t bits;
    uint16_t udp_size;
    struct edns_option* opt_list;
};
%inline %{
    struct edns_option** _edns_data_opt_list_get(struct edns_data* edns) {
       return &edns->opt_list;
    }
%}
%extend edns_data {
    %pythoncode %{
        def _opt_list_iter(self): return EdnsOptsListIter(self.opt_list)
        __swig_getmethods__["opt_list_iter"] = _opt_list_iter
        if _newclass:opt_list_iter = _swig_property(_opt_list_iter)
        def _opt_list(self): return _edns_data_opt_list_get(self)
        __swig_getmethods__["opt_list"] = _opt_list
        if _newclass:opt_list = _swig_property(_opt_list)
    %}
}

/* ************************************************************************************ *
   Structure module_env
 * ************************************************************************************ */
struct module_env {
    struct config_file* cfg;
    struct slabhash* msg_cache;
    struct rrset_cache* rrset_cache;
    struct infra_cache* infra_cache;
    struct key_cache* key_cache;

    /* --- services --- */
    struct outbound_entry* (*send_query)(struct query_info* qinfo,
        uint16_t flags, int dnssec, int want_dnssec, int nocaps,
        struct sockaddr_storage* addr, socklen_t addrlen,
        uint8_t* zone, size_t zonelen, int ssl_upstream,
        struct module_qstate* q);
    void (*detach_subs)(struct module_qstate* qstate);
    int (*attach_sub)(struct module_qstate* qstate,
        struct query_info* qinfo, uint16_t qflags, int prime,
        int valrec, struct module_qstate** newq);
    void (*kill_sub)(struct module_qstate* newq);
    int (*detect_cycle)(struct module_qstate* qstate,
        struct query_info* qinfo, uint16_t flags, int prime,
        int valrec);

    struct regional* scratch;
    struct sldns_buffer* scratch_buffer;
    struct worker* worker;
    struct mesh_area* mesh;
    struct alloc_cache* alloc;
    struct ub_randstate* rnd;
    time_t* now;
    struct timeval* now_tv;
    int need_to_validate;
    struct val_anchors* anchors;
    struct val_neg_cache* neg_cache;
    struct comm_timer* probe_timer;
    struct iter_forwards* fwds;
    struct iter_hints* hints;
    void* modinfo[MAX_MODULE];

    void* inplace_cb_lists[inplace_cb_types_total];
    struct edns_known_option* edns_known_options;
    size_t edns_known_options_num;
};

/* ************************************************************************************ *
   Structure module_qstate
 * ************************************************************************************ */
%ignore module_qstate::ext_state;
%ignore module_qstate::minfo;

/* These are ignored because we will pass a double pointer of them to Python
 * with custom getmethods. This is done to bypass Swig's behavior to pass NULL
 * pointers as None. */
%ignore module_qstate::edns_opts_front_in;
%ignore module_qstate::edns_opts_back_out;
%ignore module_qstate::edns_opts_back_in;
%ignore module_qstate::edns_opts_front_out;

/* Query state */
struct module_qstate {
   struct query_info qinfo;
   uint16_t query_flags;  /* See QF_BIT_xx constants */
   int is_priming;
   int is_valrec;

   struct comm_reply* reply;
   struct dns_msg* return_msg;
   int return_rcode;
   struct regional* region; /* unwrapped */

   int curmod;

   enum module_ext_state ext_state[MAX_MODULE];
   void* minfo[MAX_MODULE];
   time_t prefetch_leeway;

   struct module_env* env;         /* unwrapped */
   struct mesh_state* mesh_info;

   struct edns_option* edns_opts_front_in;
   struct edns_option* edns_opts_back_out;
   struct edns_option* edns_opts_back_in;
   struct edns_option* edns_opts_front_out;
   int no_cache_lookup;
   int no_cache_store;
};

%constant int MODULE_COUNT = MAX_MODULE;

%constant int QF_BIT_CD = 0x0010;
%constant int QF_BIT_AD = 0x0020;
%constant int QF_BIT_Z  = 0x0040;
%constant int QF_BIT_RA = 0x0080;
%constant int QF_BIT_RD = 0x0100;
%constant int QF_BIT_TC = 0x0200;
%constant int QF_BIT_AA = 0x0400;
%constant int QF_BIT_QR = 0x8000;

%inline %{
 enum enum_return_rcode {
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
    class ExtState:
        def __init__(self, obj): self.obj = obj
        def __str__(self):
            return ", ".join([_unboundmodule.strextstate(_unboundmodule._ext_state_get(self.obj,a)) for a in range(0, _unboundmodule.MODULE_COUNT)])
        def __getitem__(self, index): return _unboundmodule._ext_state_get(self.obj, index)
        def __setitem__(self, index, value): _unboundmodule._ext_state_set(self.obj, index, value)
        def __len__(self): return _unboundmodule.MODULE_COUNT

    class EdnsOptsListIter:
        def __init__(self, obj):
            self._current = obj
            self._temp = None
        def __iter__(self): return self
        def __next__(self):
            """Python 3 compatibility"""
            return self._get_next()
        def next(self):
            """Python 2 compatibility"""
            return self._get_next()
        def _get_next(self):
            if not edns_opt_list_is_empty(self._current):
                self._temp = self._current
                self._current = _p_p_edns_option_get_next(self._current)
                return _dereference_edns_option(self._temp)
            else:
                raise StopIteration
%}

%inline %{
   enum module_ext_state _ext_state_get(struct module_qstate* q, int idx) {
     if ((q != NULL) && (idx >= 0) && (idx < MAX_MODULE)) {
        return q->ext_state[idx];
     }
     return 0;
   }

   void _ext_state_set(struct module_qstate* q, int idx, enum module_ext_state state) {
     if ((q != NULL) && (idx >= 0) && (idx < MAX_MODULE)) {
        q->ext_state[idx] = state;
     }
   }

   int edns_opt_list_is_empty(struct edns_option** opt) {
        if (!opt || !(*opt)) return 1;
        return 0;
   }

   struct edns_option* _dereference_edns_option(struct edns_option** opt) {
        if (!opt) return NULL;
        return *opt;
   }

   struct edns_option** _p_p_edns_option_get_next(struct edns_option** opt) {
        return &(*opt)->next;
   }

   struct edns_option** _edns_opts_front_in_get(struct module_qstate* q) {
        return &q->edns_opts_front_in;
   }

   struct edns_option** _edns_opts_back_out_get(struct module_qstate* q) {
        return &q->edns_opts_back_out;
   }

   struct edns_option** _edns_opts_back_in_get(struct module_qstate* q) {
        return &q->edns_opts_back_in;
   }

   struct edns_option** _edns_opts_front_out_get(struct module_qstate* q) {
        return &q->edns_opts_front_out;
   }
%}

%extend module_qstate {
   %pythoncode %{
        def set_ext_state(self, id, state):
            """Sets the ext state"""
            _unboundmodule._ext_state_set(self, id, state)

        def __ext_state_get(self): return ExtState(self)
        __swig_getmethods__["ext_state"] = __ext_state_get
        if _newclass:ext_state = _swig_property(__ext_state_get)#, __ext_state_set)

        def _edns_opts_front_in_iter(self): return EdnsOptsListIter(self.edns_opts_front_in)
        __swig_getmethods__["edns_opts_front_in_iter"] = _edns_opts_front_in_iter
        if _newclass:edns_opts_front_in_iter = _swig_property(_edns_opts_front_in_iter)
        def _edns_opts_back_out_iter(self): return EdnsOptsListIter(self.edns_opts_back_out)
        __swig_getmethods__["edns_opts_back_out_iter"] = _edns_opts_back_out_iter
        if _newclass:edns_opts_back_out_iter = _swig_property(_edns_opts_back_out_iter)
        def _edns_opts_back_in_iter(self): return EdnsOptsListIter(self.edns_opts_back_in)
        __swig_getmethods__["edns_opts_back_in_iter"] = _edns_opts_back_in_iter
        if _newclass:edns_opts_back_in_iter = _swig_property(_edns_opts_back_in_iter)
        def _edns_opts_front_out_iter(self): return EdnsOptsListIter(self.edns_opts_front_out)
        __swig_getmethods__["edns_opts_front_out_iter"] = _edns_opts_front_out_iter
        if _newclass:edns_opts_front_out_iter = _swig_property(_edns_opts_front_out_iter)

        def _edns_opts_front_in(self): return _edns_opts_front_in_get(self)
        __swig_getmethods__["edns_opts_front_in"] = _edns_opts_front_in
        if _newclass:edns_opts_front_in = _swig_property(_edns_opts_front_in)
        def _edns_opts_back_out(self): return _edns_opts_back_out_get(self)
        __swig_getmethods__["edns_opts_back_out"] = _edns_opts_back_out
        if _newclass:edns_opts_back_out = _swig_property(_edns_opts_back_out)
        def _edns_opts_back_in(self): return _edns_opts_back_in_get(self)
        __swig_getmethods__["edns_opts_back_in"] = _edns_opts_back_in
        if _newclass:edns_opts_back_in = _swig_property(_edns_opts_back_in)
        def _edns_opts_front_out(self): return _edns_opts_front_out_get(self)
        __swig_getmethods__["edns_opts_front_out"] = _edns_opts_front_out
        if _newclass:edns_opts_front_out = _swig_property(_edns_opts_front_out)
   %}
}

/* ************************************************************************************ *
   Structure config_strlist
 * ************************************************************************************ */
struct config_strlist {
   struct config_strlist* next;
   char* str;
};

/* ************************************************************************************ *
   Structure config_str2list
 * ************************************************************************************ */
struct config_str2list {
   struct config_str2list* next;
   char* str;
   char* str2;
};

/* ************************************************************************************ *
   Structure config_file
 * ************************************************************************************ */
struct config_file {
   int verbosity;
   int stat_interval;
   int stat_cumulative;
   int stat_extended;
   int num_threads;
   int port;
   int do_ip4;
   int do_ip6;
   int do_udp;
   int do_tcp;
   int outgoing_num_ports;
   size_t outgoing_num_tcp;
   size_t incoming_num_tcp;
   int* outgoing_avail_ports;
   size_t msg_buffer_size;
   size_t msg_cache_size;
   size_t msg_cache_slabs;
   size_t num_queries_per_thread;
   size_t jostle_time;
   size_t rrset_cache_size;
   size_t rrset_cache_slabs;
   int host_ttl;
   size_t infra_cache_slabs;
   size_t infra_cache_numhosts;
   char* target_fetch_policy;
   int if_automatic;
   int num_ifs;
   char **ifs;
   int num_out_ifs;
   char **out_ifs;
   struct config_strlist* root_hints;
   struct config_stub* stubs;
   struct config_stub* forwards;
   struct config_strlist* donotqueryaddrs;
   struct config_str2list* acls;
   int donotquery_localhost;
   int harden_short_bufsize;
   int harden_large_queries;
   int harden_glue;
   int harden_dnssec_stripped;
   int harden_referral_path;
   int use_caps_bits_for_id;
   struct config_strlist* private_address;
   struct config_strlist* private_domain;
   size_t unwanted_threshold;
   char* chrootdir;
   char* username;
   char* directory;
   char* logfile;
   char* pidfile;
   int use_syslog;
   int hide_identity;
   int hide_version;
   char* identity;
   char* version;
   char* module_conf;
   struct config_strlist* trust_anchor_file_list;
   struct config_strlist* trust_anchor_list;
   struct config_strlist* trusted_keys_file_list;
   char* dlv_anchor_file;
   struct config_strlist* dlv_anchor_list;
   int max_ttl;
   int32_t val_date_override;
   int bogus_ttl;
   int val_clean_additional;
   int val_permissive_mode;
   char* val_nsec3_key_iterations;
   size_t key_cache_size;
   size_t key_cache_slabs;
   size_t neg_cache_size;
   struct config_str2list* local_zones;
   struct config_strlist* local_zones_nodefault;
   struct config_strlist* local_data;
   int remote_control_enable;
   struct config_strlist* control_ifs;
   int control_port;
   char* server_key_file;
   char* server_cert_file;
   char* control_key_file;
   char* control_cert_file;
   int do_daemonize;
   char* python_script;
};

/* ************************************************************************************ *
   ASN: Adding structures related to forwards_lookup and dns_cache_find_delegation
 * ************************************************************************************ */
struct delegpt_ns {
    struct delegpt_ns* next;
    int resolved;
    uint8_t got4;
    uint8_t got6;
    uint8_t lame;
    uint8_t done_pside4;
    uint8_t done_pside6;
};

struct delegpt_addr {
    struct delegpt_addr* next_result;
    struct delegpt_addr* next_usable;
    struct delegpt_addr* next_target;
    int attempts;
    int sel_rtt;
    int bogus;
    int lame;
};

struct delegpt {
    int namelabs;
    struct delegpt_ns* nslist;
    struct delegpt_addr* target_list;
    struct delegpt_addr* usable_list;
    struct delegpt_addr* result_list;
    int bogus;
    uint8_t has_parent_side_NS;
    uint8_t dp_type_mlc;
};


%inline %{
   PyObject* _get_dp_dname(struct delegpt* dp) {
      return PyBytes_FromStringAndSize((char*)dp->name, dp->namelen);
   }
   PyObject* _get_dp_dname_components(struct delegpt* dp) {
      return GetNameAsLabelList((char*)dp->name, dp->namelen);
   }
   PyObject* _get_dpns_dname(struct delegpt_ns* dpns) {
      return PyBytes_FromStringAndSize((char*)dpns->name, dpns->namelen);
   }
   PyObject* _get_dpns_dname_components(struct delegpt_ns* dpns) {
      return GetNameAsLabelList((char*)dpns->name, dpns->namelen);
   }

  PyObject* _delegpt_addr_addr_get(struct delegpt_addr* target) {
     char dest[64];
     delegpt_addr_addr2str(target, dest, 64);
     if (dest[0] == 0)
        return Py_None;
     return PyBytes_FromString(dest);
  }

%}

%extend delegpt {
   %pythoncode %{
        __swig_getmethods__["dname"] = _unboundmodule._get_dp_dname
        if _newclass:dname = _swig_property(_unboundmodule._get_dp_dname)

        __swig_getmethods__["dname_list"] = _unboundmodule._get_dp_dname_components
        if _newclass:dname_list = _swig_property(_unboundmodule._get_dp_dname_components)

        def _get_dname_str(self): return dnameAsStr(self.dname)
        __swig_getmethods__["dname_str"] = _get_dname_str
        if _newclass:dname_str = _swig_property(_get_dname_str)
   %}
}
%extend delegpt_ns {
   %pythoncode %{
        __swig_getmethods__["dname"] = _unboundmodule._get_dpns_dname
        if _newclass:dname = _swig_property(_unboundmodule._get_dpns_dname)

        __swig_getmethods__["dname_list"] = _unboundmodule._get_dpns_dname_components
        if _newclass:dname_list = _swig_property(_unboundmodule._get_dpns_dname_components)

        def _get_dname_str(self): return dnameAsStr(self.dname)
        __swig_getmethods__["dname_str"] = _get_dname_str
        if _newclass:dname_str = _swig_property(_get_dname_str)
   %}
}
%extend delegpt_addr {
   %pythoncode %{
        def _addr_get(self): return _delegpt_addr_addr_get(self)
        __swig_getmethods__["addr"] = _addr_get
        if _newclass:addr = _swig_property(_addr_get)
   %}
}

/* ************************************************************************************ *
   Enums
 * ************************************************************************************ */
%rename ("MODULE_STATE_INITIAL") "module_state_initial";
%rename ("MODULE_WAIT_REPLY") "module_wait_reply";
%rename ("MODULE_WAIT_MODULE") "module_wait_module";
%rename ("MODULE_WAIT_SUBQUERY") "module_wait_subquery";
%rename ("MODULE_ERROR") "module_error";
%rename ("MODULE_FINISHED") "module_finished";

enum module_ext_state {
   module_state_initial = 0,
   module_wait_reply,
   module_wait_module,
   module_wait_subquery,
   module_error,
   module_finished
};

%rename ("MODULE_EVENT_NEW") "module_event_new";
%rename ("MODULE_EVENT_PASS") "module_event_pass";
%rename ("MODULE_EVENT_REPLY") "module_event_reply";
%rename ("MODULE_EVENT_NOREPLY") "module_event_noreply";
%rename ("MODULE_EVENT_CAPSFAIL") "module_event_capsfail";
%rename ("MODULE_EVENT_MODDONE") "module_event_moddone";
%rename ("MODULE_EVENT_ERROR") "module_event_error";

enum module_ev {
   module_event_new = 0,
   module_event_pass,
   module_event_reply,
   module_event_noreply,
   module_event_capsfail,
   module_event_moddone,
   module_event_error
};

enum sec_status {
   sec_status_unchecked = 0,
   sec_status_bogus,
   sec_status_indeterminate,
   sec_status_insecure,
   sec_status_secure
};

enum verbosity_value {
   NO_VERBOSE = 0,
   VERB_OPS,
   VERB_DETAIL,
   VERB_QUERY,
   VERB_ALGO
};

enum inplace_cb_list_type {
    /* Inplace callbacks for when a resolved reply is ready to be sent to the
     * front.*/
    inplace_cb_reply = 0,
    /* Inplace callbacks for when a reply is given from the cache. */
    inplace_cb_reply_cache,
    /* Inplace callbacks for when a reply is given with local data
     * (or Chaos reply). */
    inplace_cb_reply_local,
    /* Inplace callbacks for when the reply is servfail. */
    inplace_cb_reply_servfail,
    /* Inplace callbacks for when a query is ready to be sent to the back.*/
    inplace_cb_query,
    /* Inplace callback for when a reply is received from the back. */
    inplace_cb_edns_back_parsed,
    /* Total number of types. Used for array initialization.
     * Should always be last. */
    inplace_cb_types_total
};

%constant uint16_t PKT_QR = 1;      /* QueRy - query flag */
%constant uint16_t PKT_AA = 2;      /* Authoritative Answer - server flag */
%constant uint16_t PKT_TC = 4;      /* TrunCated - server flag */
%constant uint16_t PKT_RD = 8;      /* Recursion Desired - query flag */
%constant uint16_t PKT_CD = 16;     /* Checking Disabled - query flag */
%constant uint16_t PKT_RA = 32;     /* Recursion Available - server flag */
%constant uint16_t PKT_AD = 64;     /* Authenticated Data - server flag */

%{
int checkList(PyObject *l)
{
    PyObject* item;
    int i;

    if (l == Py_None)
       return 1;

    if (PyList_Check(l))
    {
       for (i=0; i < PyList_Size(l); i++)
       {
           item = PyList_GetItem(l, i);
           if (!PyBytes_Check(item))
              return 0;
       }
       return 1;
    }

    return 0;
}

int pushRRList(sldns_buffer* qb, PyObject *l, uint32_t default_ttl, int qsec,
        size_t count_offset)
{
    PyObject* item;
    int i;
    size_t len;

    for (i=0; i < PyList_Size(l); i++)
    {
        item = PyList_GetItem(l, i);

        len = sldns_buffer_remaining(qb);
        if(qsec) {
                if(sldns_str2wire_rr_question_buf(PyBytes_AsString(item),
                        sldns_buffer_current(qb), &len, NULL, NULL, 0, NULL, 0)
                        != 0)
                        return 0;
        } else {
                if(sldns_str2wire_rr_buf(PyBytes_AsString(item),
                        sldns_buffer_current(qb), &len, NULL, default_ttl,
                        NULL, 0, NULL, 0) != 0)
                        return 0;
        }
        sldns_buffer_skip(qb, len);

        sldns_buffer_write_u16_at(qb, count_offset,
                sldns_buffer_read_u16_at(qb, count_offset)+1);
    }
    return 1;
}

int set_return_msg(struct module_qstate* qstate,
                   const char* rr_name, sldns_rr_type rr_type, sldns_rr_class rr_class , uint16_t flags, uint32_t default_ttl,
                   PyObject* question, PyObject* answer, PyObject* authority, PyObject* additional)
{
     sldns_buffer *qb = 0;
     int res = 1;
     size_t l;
     uint16_t PKT_QR = 1;
     uint16_t PKT_AA = 2;
     uint16_t PKT_TC = 4;
     uint16_t PKT_RD = 8;
     uint16_t PKT_CD = 16;
     uint16_t PKT_RA = 32;
     uint16_t PKT_AD = 64;

     if ((!checkList(question)) || (!checkList(answer)) || (!checkList(authority)) || (!checkList(additional)))
        return 0;
     if ((qb = sldns_buffer_new(LDNS_RR_BUF_SIZE)) == 0) return 0;

     /* write header */
     sldns_buffer_write_u16(qb, 0); /* ID */
     sldns_buffer_write_u16(qb, 0); /* flags */
     sldns_buffer_write_u16(qb, 1); /* qdcount */
     sldns_buffer_write_u16(qb, 0); /* ancount */
     sldns_buffer_write_u16(qb, 0); /* nscount */
     sldns_buffer_write_u16(qb, 0); /* arcount */
     if ((flags&PKT_QR)) LDNS_QR_SET(sldns_buffer_begin(qb));
     if ((flags&PKT_AA)) LDNS_AA_SET(sldns_buffer_begin(qb));
     if ((flags&PKT_TC)) LDNS_TC_SET(sldns_buffer_begin(qb));
     if ((flags&PKT_RD)) LDNS_RD_SET(sldns_buffer_begin(qb));
     if ((flags&PKT_CD)) LDNS_CD_SET(sldns_buffer_begin(qb));
     if ((flags&PKT_RA)) LDNS_RA_SET(sldns_buffer_begin(qb));
     if ((flags&PKT_AD)) LDNS_AD_SET(sldns_buffer_begin(qb));

     /* write the query */
     l = sldns_buffer_remaining(qb);
     if(sldns_str2wire_dname_buf(rr_name, sldns_buffer_current(qb), &l) != 0) {
             sldns_buffer_free(qb);
             return 0;
     }
     sldns_buffer_skip(qb, l);
     if (rr_type == 0) { rr_type = LDNS_RR_TYPE_A; }
     if (rr_class == 0) { rr_class = LDNS_RR_CLASS_IN; }
     sldns_buffer_write_u16(qb, rr_type);
     sldns_buffer_write_u16(qb, rr_class);

     /* write RR sections */
     if(res && !pushRRList(qb, question, default_ttl, 1, LDNS_QDCOUNT_OFF))
             res = 0;
     if(res && !pushRRList(qb, answer, default_ttl, 0, LDNS_ANCOUNT_OFF))
             res = 0;
     if(res && !pushRRList(qb, authority, default_ttl, 0, LDNS_NSCOUNT_OFF))
             res = 0;
     if(res && !pushRRList(qb, additional, default_ttl, 0, LDNS_ARCOUNT_OFF))
             res = 0;

     if (res) res = createResponse(qstate, qb);

     if (qb) sldns_buffer_free(qb);
     return res;
}
%}

int set_return_msg(struct module_qstate* qstate,
                   const char* rr_name, int rr_type, int rr_class , uint16_t flags, uint32_t default_ttl,
                   PyObject* question, PyObject* answer, PyObject* authority, PyObject* additional);

%pythoncode %{
    class DNSMessage:
        def __init__(self, rr_name, rr_type, rr_class = RR_CLASS_IN, query_flags = 0, default_ttl = 0):
            """Query flags is a combination of PKT_xx contants"""
            self.rr_name = rr_name
            self.rr_type = rr_type
            self.rr_class = rr_class
            self.default_ttl = default_ttl
            self.query_flags = query_flags
            self.question = []
            self.answer = []
            self.authority = []
            self.additional = []

        def set_return_msg(self, qstate):
            """Returns 1 if OK"""
            status = _unboundmodule.set_return_msg(qstate, self.rr_name, self.rr_type, self.rr_class,
                                           self.query_flags, self.default_ttl,
                                           self.question, self.answer, self.authority, self.additional)

            if (status) and (PKT_AA & self.query_flags):
                qstate.return_msg.rep.authoritative = 1

            return status

%}
/* ************************************************************************************ *
   ASN: Delegation pointer related functions
 * ************************************************************************************ */

/* Functions which we will need to lookup delegations */
struct delegpt* dns_cache_find_delegation(struct module_env* env,
        uint8_t* qname, size_t qnamelen, uint16_t qtype, uint16_t qclass,
        struct regional* region, struct dns_msg** msg, uint32_t timenow);
int iter_dp_is_useless(struct query_info* qinfo, uint16_t qflags,
        struct delegpt* dp);
struct iter_hints_stub* hints_lookup_stub(struct iter_hints* hints,
        uint8_t* qname, uint16_t qclass, struct delegpt* dp);

/* Custom function to perform logic similar to the one in daemon/cachedump.c */
struct delegpt* find_delegation(struct module_qstate* qstate, char *nm, size_t nmlen);

%{
#define BIT_RD 0x100

struct delegpt* find_delegation(struct module_qstate* qstate, char *nm, size_t nmlen)
{
    struct delegpt *dp;
    struct dns_msg *msg = NULL;
    struct regional* region = qstate->env->scratch;
    char b[260];
    struct query_info qinfo;
    struct iter_hints_stub* stub;
    uint32_t timenow = *qstate->env->now;

    regional_free_all(region);
    qinfo.qname = (uint8_t*)nm;
    qinfo.qname_len = nmlen;
    qinfo.qtype = LDNS_RR_TYPE_A;
    qinfo.qclass = LDNS_RR_CLASS_IN;

    while(1) {
        dp = dns_cache_find_delegation(qstate->env, (uint8_t*)nm, nmlen, qinfo.qtype, qinfo.qclass, region, &msg, timenow);
        if(!dp)
            return NULL;
        if(iter_dp_is_useless(&qinfo, BIT_RD, dp)) {
            if (dname_is_root((uint8_t*)nm))
                return NULL;
            nm = (char*)dp->name;
            nmlen = dp->namelen;
            dname_remove_label((uint8_t**)&nm, &nmlen);
            dname_str((uint8_t*)nm, b);
            continue;
        }
        stub = hints_lookup_stub(qstate->env->hints, qinfo.qname, qinfo.qclass, dp);
        if (stub) {
            return stub->dp;
        } else {
            return dp;
        }
    }
    return NULL;
}
%}

/* ************************************************************************************ *
   Functions
 * ************************************************************************************ */
/******************************
 * Various debuging functions *
 ******************************/
void verbose(enum verbosity_value level, const char* format, ...);
void log_info(const char* format, ...);
void log_err(const char* format, ...);
void log_warn(const char* format, ...);
void log_hex(const char* msg, void* data, size_t length);
void log_dns_msg(const char* str, struct query_info* qinfo, struct reply_info* rep);
void log_query_info(enum verbosity_value v, const char* str, struct query_info* qinf);
void regional_log_stats(struct regional *r);

/***************************************************************************
 * Free allocated memory from marked sources returning corresponding types *
 ***************************************************************************/
%typemap(newfree, noblock = 1) char * {
  free($1);
}

/***************************************************
 * Mark as source returning newly allocated memory *
 ***************************************************/
%newobject sldns_wire2str_type;
%newobject sldns_wire2str_class;

/******************
 * LDNS functions *
 ******************/
char *sldns_wire2str_type(const uint16_t atype);
char *sldns_wire2str_class(const uint16_t aclass);

/**********************************
 * Functions from pythonmod_utils *
 **********************************/
int storeQueryInCache(struct module_qstate* qstate, struct query_info* qinfo, struct reply_info* msgrep, int is_referral);
void invalidateQueryInCache(struct module_qstate* qstate, struct query_info* qinfo);

/*******************************
 * Module conversion functions *
 *******************************/
const char* strextstate(enum module_ext_state s);
const char* strmodulevent(enum module_ev e);

/**************************
 * Edns related functions *
 **************************/
struct edns_option* edns_opt_list_find(struct edns_option* list, uint16_t code);
int edns_register_option(uint16_t opt_code, int bypass_cache_stage,
    int no_aggregation, struct module_env* env);

%pythoncode %{
    def register_edns_option(env, code, bypass_cache_stage=False,
                             no_aggregation=False):
        """Wrapper function to provide keyword attributes."""
        return edns_register_option(code, bypass_cache_stage,
                                    no_aggregation, env)
%}

/******************************
 * Callback related functions *
 ******************************/
/* typemap to check if argument is callable */
%typemap(in) PyObject *py_cb {
  if (!PyCallable_Check($input)) {
      SWIG_exception_fail(SWIG_TypeError, "Need a callable object!");
      return NULL;
  }
  $1 = $input;
}
/* typemap to get content/size from a bytearray  */
%typemap(in) (size_t len, uint8_t* py_bytearray_data) {
    if (!PyByteArray_CheckExact($input)) {
        SWIG_exception_fail(SWIG_TypeError, "Expected bytearray!");
        return NULL;
    }
    $2 = (void*)PyByteArray_AsString($input);
    $1 = PyByteArray_Size($input);
}

int edns_opt_list_remove(struct edns_option** list, uint16_t code);
int edns_opt_list_append(struct edns_option** list, uint16_t code, size_t len,
    uint8_t* py_bytearray_data, struct regional* region);

%{
    /* This function is called by unbound in order to call the python
     * callback function. */
    int python_inplace_cb_reply_generic(struct query_info* qinfo,
        struct module_qstate* qstate, struct reply_info* rep, int rcode,
        struct edns_data* edns, struct edns_option** opt_list_out,
        struct regional* region, int id, void* python_callback)
    {
        PyObject *func, *py_edns, *py_qstate, *py_opt_list_out, *py_qinfo;
        PyObject *py_rep, *py_region;
        PyObject *result;
        int res = 0;

        PyGILState_STATE gstate = PyGILState_Ensure();
        func = (PyObject *) python_callback;
        py_edns = SWIG_NewPointerObj((void*) edns, SWIGTYPE_p_edns_data, 0);
        py_qstate = SWIG_NewPointerObj((void*) qstate,
            SWIGTYPE_p_module_qstate, 0);
        py_opt_list_out = SWIG_NewPointerObj((void*) opt_list_out,
            SWIGTYPE_p_p_edns_option, 0);
        py_qinfo = SWIG_NewPointerObj((void*) qinfo, SWIGTYPE_p_query_info, 0);
        py_rep = SWIG_NewPointerObj((void*) rep, SWIGTYPE_p_reply_info, 0);
        py_region = SWIG_NewPointerObj((void*) region, SWIGTYPE_p_regional, 0);
        result = PyObject_CallFunction(func, "OOOiOOO", py_qinfo, py_qstate,
            py_rep, rcode, py_edns, py_opt_list_out, py_region);
        Py_XDECREF(py_edns);
        Py_XDECREF(py_qstate);
        Py_XDECREF(py_opt_list_out);
        Py_XDECREF(py_qinfo);
        Py_XDECREF(py_rep);
        Py_XDECREF(py_region);
        if (result) {
            res = PyInt_AsLong(result);
        }
        Py_XDECREF(result);
        PyGILState_Release(gstate);
        return res;
    }

    /* register a callback */
    static int python_inplace_cb_register(enum inplace_cb_list_type type,
        PyObject* py_cb, struct module_env* env, int id)
    {
        int ret = inplace_cb_register(python_inplace_cb_reply_generic,
                type, (void*) py_cb, env, id);
        if (ret) Py_INCREF(py_cb);
        return ret;
    }

    /* Swig implementations for Python */
    static int register_inplace_cb_reply(PyObject* py_cb,
        struct module_env* env, int id)
    {
        return python_inplace_cb_register(inplace_cb_reply, py_cb, env, id);
    }
    static int register_inplace_cb_reply_cache(PyObject* py_cb,
        struct module_env* env, int id)
    {
        return python_inplace_cb_register(inplace_cb_reply_cache, py_cb, env, id);
    }
    static int register_inplace_cb_reply_local(PyObject* py_cb,
        struct module_env* env, int id)
    {
        return python_inplace_cb_register(inplace_cb_reply_local, py_cb, env, id);
    }
    static int register_inplace_cb_reply_servfail(PyObject* py_cb,
        struct module_env* env, int id)
    {
        return python_inplace_cb_register(inplace_cb_reply_servfail,
                py_cb, env, id);
    }
%}
/* C declarations */
int inplace_cb_register(void* cb, enum inplace_cb_list_type type, void* cbarg,
    struct module_env* env, int id);

/* Swig declarations */
static int register_inplace_cb_reply(PyObject* py_cb,
    struct module_env* env, int id);
static int register_inplace_cb_reply_cache(PyObject* py_cb,
    struct module_env* env, int id);
static int register_inplace_cb_reply_local(PyObject* py_cb,
    struct module_env* env, int id);
static int register_inplace_cb_reply_servfail(PyObject* py_cb,
    struct module_env* env, int id);
