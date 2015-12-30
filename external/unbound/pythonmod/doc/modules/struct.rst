Scriptable structures
=====================

module_qstate
-----------------------

.. class:: module_qstate

   Module state, per query.
   
   This class provides these data attributes:
   
   .. attribute:: qinfo
   
      (:class:`query_info`) Informations about query being answered. Name, RR type, RR class.
   
   .. attribute:: query_flags
   
      (uint16) Flags for query. See QF_BIT\_ predefined constants.
      
   .. attribute:: is_priming
   
      If this is a (stub or root) priming query (with hints).
   
   .. attribute:: reply
   
      comm_reply contains server replies.
      
   .. attribute:: return_msg
   
      (:class:`dns_msg`) The reply message, with message for client and calling module (read-only attribute).
		Note that if you want to create of modify return_msg you should use :class:`DNSMessage`.
      
   .. attribute:: return_rcode
   
      The rcode, in case of error, instead of a reply message. Determines whether the return_msg contains reply.
   
   .. attribute:: region
   
      Region for this query. Cleared when query process finishes.
   
   .. attribute:: curmod
   
      Which module is executing.
      
   .. attribute:: ext_state[]
   
      Module states.
      
   .. attribute:: env
   
      Environment for this query.
      
   .. attribute:: mesh_info
   
      Mesh related information for this query.


query_info
----------------

.. class:: query_info

   This class provides these data attributes:

   .. attribute:: qname
   
      The original question in the wireformat format (e.g. \\x03www\\x03nic\\x02cz\\x00 for www.nic.cz)
   
   .. attribute:: qname_len
   
      Lenght of question name (number of bytes).
	
   .. attribute:: qname_list[]
   
      The question ``qname`` converted into list of labels (e.g. ['www','nic','cz',''] for www.nic.cz)
   
   .. attribute:: qname_str
   
      The question ``qname`` converted into string (e.g. www.nic.cz. for www.nic.cz)

   .. attribute:: qtype
   
      The class type asked for. See RR_TYPE\_ predefined constants.
   
   .. attribute:: qtype_str
   
      The ``qtype`` in display presentation format (string) (e.g 'A' for RR_TYPE_A)

   .. attribute:: qclass
   
      The question class. See RR_CLASS\_ predefined constants.
   
   .. attribute:: qclass_str
   
      The ``qclass`` in display presentation format (string).
   
reply_info
--------------------

.. class:: reply_info

   This class provides these data attributes:

   .. attribute:: flags
   
      The flags for the answer, host byte order.
   
   .. attribute:: qdcount
   
      Number of RRs in the query section.
      If qdcount is not 0, then it is 1, and the data that appears
      in the reply is the same as the query_info.
      Host byte order.
   
   .. attribute:: ttl
   
      TTL of the entire reply (for negative caching).
      only for use when there are 0 RRsets in this message.
      if there are RRsets, check those instead.
   
   .. attribute:: security
   
      The security status from DNSSEC validation of this message. See sec_status\_ predefined constants.
   
   .. attribute:: an_numrrsets
   
      Number of RRsets in each section.
      The answer section. Add up the RRs in every RRset to calculate
      the number of RRs, and the count for the dns packet. 
      The number of RRs in RRsets can change due to RRset updates.
   
   .. attribute:: ns_numrrsets
   
      Count of authority section RRsets
   
   .. attribute:: ar_numrrsets
   
      Count of additional section RRsets 
   
   .. attribute:: rrset_count
   
      Number of RRsets: an_numrrsets + ns_numrrsets + ar_numrrsets 
   
   .. attribute:: rrsets[]
   
         (:class:`ub_packed_rrset_key`) List of RR sets in the order in which they appear in the reply message.  
         Number of elements is ancount + nscount + arcount RRsets.
   
   .. attribute:: ref[]
   
         (:class:`rrset_ref`) Packed array of ids (see counts) and pointers to packed_rrset_key.
         The number equals ancount + nscount + arcount RRsets. 
         These are sorted in ascending pointer, the locking order. So
         this list can be locked (and id, ttl checked), to see if 
         all the data is available and recent enough.
   

dns_msg
--------------

.. class:: dns_msg

   Region allocated message reply

   This class provides these data attributes:

   .. attribute:: qinfo
   
      (:class:`query_info`) Informations about query.
   
   .. attribute:: rep
   
      (:class:`reply_info`) This attribute points to the packed reply structure.


packed_rrset_key
----------------------
   
.. class:: packed_rrset_key

   The identifying information for an RRset.

   This class provides these data attributes:

   .. attribute:: dname
   
      The domain name. If not empty (for ``id = None``) it is allocated, and
      contains the wireformat domain name. This dname is not canonicalized.
      E.g., the dname contains \\x03www\\x03nic\\x02cz\\x00 for www.nic.cz.
   
   .. attribute:: dname_len
   
      Length of the domain name, including last 0 root octet. 
      
   .. attribute:: dname_list[]
   
      The domain name ``dname`` converted into list of labels (see :attr:`query_info.qname_list`).
   
   .. attribute:: dname_str
   
      The domain name ``dname`` converted into string (see :attr:`query_info.qname_str`).

   .. attribute:: flags
   
      Flags.
      
   .. attribute:: type
   
      The rrset type in network format.

   .. attribute:: type_str
   
      The rrset type in display presentation format.
      
   .. attribute:: rrset_class
   
      The rrset class in network format.

   .. attribute:: rrset_class_str
   
      The rrset class in display presentation format.

ub_packed_rrset_key
-------------------------

.. class:: ub_packed_rrset_key

   This structure contains an RRset. A set of resource records that
   share the same domain name, type and class.
   Due to memory management and threading, the key structure cannot be
   deleted, although the data can be. The id can be set to 0 to store and the
   structure can be recycled with a new id.
   
   The :class:`ub_packed_rrset_key` provides these data attributes:
   
   .. attribute:: entry
      
      (:class:`lruhash_entry`) Entry into hashtable. Note the lock is never destroyed,
      even when this key is retired to the cache. 
      the data pointer (if not None) points to a :class:`packed_rrset`.
    
   .. attribute:: id
      
      The ID of this rrset. unique, based on threadid + sequenceno. 
      ids are not reused, except after flushing the cache.
      zero is an unused entry, and never a valid id.
      Check this value after getting entry.lock.
      The other values in this struct may only be altered after changing
      the id (which needs a writelock on entry.lock).
      
   .. attribute:: rk
   
      (:class:`packed_rrset_key`) RR set data.


lruhash_entry
-------------------------

.. class:: lruhash_entry

   The :class:`ub_packed_rrset_key` provides these data attributes:

   .. attribute:: lock

      rwlock for access to the contents of the entry. Note that you cannot change hash and key, if so, you have to delete it to change hash or key.

   .. attribute:: data

      (:class:`packed_rrset_data`) entry data stored in wireformat (RRs and RRsigs).

packed_rrset_data
-----------------------
   
.. class:: packed_rrset_data

   Rdata is stored in wireformat. The dname is stored in wireformat.
   
   TTLs are stored as absolute values (and could be expired).
   
   RRSIGs are stored in the arrays after the regular rrs.
   
   You need the packed_rrset_key to know dname, type, class of the
   resource records in this RRset. (if signed the rrsig gives the type too).

   The :class:`packed_rrset_data` provides these data attributes:

   .. attribute:: ttl
   
      TTL (in seconds like time()) of the RRset.
      Same for all RRs see rfc2181(5.2).
   
   .. attribute:: count
      
      Number of RRs.
   
   .. attribute:: rrsig_count
      
      Number of rrsigs, if 0 no rrsigs.
      
   .. attribute:: trust
   
      The trustworthiness of the RRset data.
      
   .. attribute:: security
   
      Security status of the RRset data. See sec_status\_ predefined constants.
      
   .. attribute:: rr_len[]
   
      Length of every RR's rdata, rr_len[i] is size of rr_data[i].
      
   .. attribute:: rr_ttl[]
   
      TTL of every rr. rr_ttl[i] ttl of rr i.
      
   .. attribute:: rr_data[]
   
      Array of RR's rdata (list of strings). The rdata is stored in uncompressed wireformat. 
      The first 16B of rr_data[i] is rdlength in network format.
   

DNSMessage
----------------
   
.. class:: DNSMessage

   Abstract representation of DNS message.
   
   **Usage**

      This example shows how to create an authoritative answer response
		
      ::

         msg = DNSMessage(qstate.qinfo.qname_str, RR_TYPE_A, RR_CLASS_IN, PKT_AA)

         #append RR
         if (qstate.qinfo.qtype == RR_TYPE_A) or (qstate.qinfo.qtype == RR_TYPE_ANY):
             msg.answer.append("%s 10 IN A 127.0.0.1" % qstate.qinfo.qname_str)
         
         #set qstate.return_msg 
         if not msg.set_return_msg(qstate):
             raise Exception("Can't create response")

   The :class:`DNSMessage` provides these methods and data attributes:
   
   .. method:: __init__(self, rr_name, rr_type, rr_class = RR_CLASS_IN, query_flags = 0, default_ttl = 0)
   
      Prepares an answer (DNS packet) from given information. Query flags are combination of PKT_xx constants.
      
   .. method:: set_return_msg(self, qstate)
   
      This method fills qstate return message according to the given informations. 
		It takes lists of RRs in each section of answer, created necessary RRsets in wire format and store the result in :attr:`qstate.return_msg`.
		Returns 1 if OK.
   
   .. attribute:: rr_name
   
      RR name of question.
      
   .. attribute:: rr_type
   
      RR type of question.
      
   .. attribute:: rr_class
   
      RR class of question.
      
   .. attribute:: default_ttl
   
      Default time-to-live.
      
   .. attribute:: query_flags
   
      Query flags. See PKT\_ predefined constants.
      
   .. attribute:: question[]
   
      List of resource records that should appear (in the same order) in question section of answer.
      
   .. attribute:: answer[]
   
      List of resource records that should appear (in the same order) in answer section of answer.
     
   .. attribute:: authority[]
   
      List of resource records that should appear (in the same order) in authority section of answer.
      
   .. attribute:: additional[]
   
      List of resource records that should appear (in the same order) in additional section of answer.

pythonmod_env
-----------------------

.. class:: pythonmod_env

   Global state for the module. 

   This class provides these data attributes:

   .. attribute:: data
   
      Here you can keep your own data shared across each thread.

   .. attribute:: fname
   
   	Python script filename.
   
   .. attribute:: qstate
   
      Module query state.

pythonmod_qstate
-----------------------

.. class:: pythonmod_qstate

   Per query state for the iterator module.
	
   This class provides these data attributes:
	
   .. attribute:: data
	
	   Here you can keep your own private data (each thread has own data object).

