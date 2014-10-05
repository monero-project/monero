Global environment
==================

Global variables
----------------

.. envvar:: mod_env

   Module environment, contains data pointer for module-specific data.
   See :class:`pythonmod_env`.


Predefined constants
-----------------------

Module extended state
~~~~~~~~~~~~~~~~~~~~~~~

.. data:: module_state_initial

   Initial state - new DNS query.

.. data:: module_wait_reply

   Waiting for reply to outgoing network query.

.. data:: module_wait_module

   Module is waiting for another module.
   
.. data:: module_wait_subquery

   Module is waiting for sub-query.
   
.. data:: module_error

   Module could not finish the query.
   
.. data:: module_finished

   Module is finished with query.

Module event
~~~~~~~~~~~~~
.. data:: module_event_new

   New DNS query.
   
.. data:: module_event_pass

   Query passed by other module.
   
.. data:: module_event_reply

   Reply inbound from server.
   
.. data:: module_event_noreply

   No reply, timeout or other error.
   
.. data:: module_event_capsfail

   Reply is there, but capitalisation check failed.
   
.. data:: module_event_moddone

   Next module is done, and its reply is awaiting you.
   
.. data:: module_event_error

   Error occured.

Security status
~~~~~~~~~~~~~~~~

.. data:: sec_status_unchecked

   Means that object has yet to be validated.

.. data:: sec_status_bogus

   Means that the object *(RRset or message)* failed to validate
   *(according to local policy)*, but should have validated.
   
.. data:: sec_status_indeterminate

   Means that the object is insecure, but not 
   authoritatively so. Generally this means that the RRset is not 
   below a configured trust anchor.
   
.. data:: sec_status_insecure

   Means that the object is authoritatively known to be 
   insecure. Generally this means that this RRset is below a trust 
   anchor, but also below a verified, insecure delegation.

.. data:: sec_status_secure

   Means that the object (RRset or message) validated according to local policy.

Resource records (RR sets)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The different RR classes.

   .. data:: RR_CLASS_IN
   
      Internet.
      
   .. data:: RR_CLASS_CH
   
      Chaos.
      
   .. data:: RR_CLASS_HS
   
      Hesiod (Dyer 87)
      
   .. data:: RR_CLASS_NONE
   
      None class, dynamic update.
      
   .. data:: RR_CLASS_ANY
      
      Any class.
   

The different RR types.


   .. data:: RR_TYPE_A 
   
      A host address.
      
   .. data:: RR_TYPE_NS
   
      An authoritative name server.
      
   .. data:: RR_TYPE_MD 
      
      A mail destination (Obsolete - use MX).
      
   .. data:: RR_TYPE_MF 
   
      A mail forwarder (Obsolete - use MX).
      
   .. data:: RR_TYPE_CNAME 
      
      The canonical name for an alias.
      
   .. data:: RR_TYPE_SOA 
      
      Marks the start of a zone of authority.
      
   .. data:: RR_TYPE_MB 
      
      A mailbox domain name (EXPERIMENTAL).
      
   .. data:: RR_TYPE_MG 
      
      A mail group member (EXPERIMENTAL).
      
   .. data:: RR_TYPE_MR 
      
      A mail rename domain name (EXPERIMENTAL).
      
   .. data:: RR_TYPE_NULL
      
      A null RR (EXPERIMENTAL).
      
   .. data:: RR_TYPE_WKS
      
      A well known service description.
      
   .. data:: RR_TYPE_PTR
   
      A domain name pointer.
      
   .. data:: RR_TYPE_HINFO
   
      Host information.
      
   .. data:: RR_TYPE_MINFO
   
      Mailbox or mail list information.
      
   .. data:: RR_TYPE_MX
   
      Mail exchange.
      
   .. data:: RR_TYPE_TXT
   
      Text strings.
   
   .. data:: RR_TYPE_RP
   
      RFC1183.
      
   .. data:: RR_TYPE_AFSDB
      
      RFC1183.
      
   .. data:: RR_TYPE_X25
      
      RFC1183.
      
   .. data:: RR_TYPE_ISDN
   
      RFC1183.
      
   .. data:: RR_TYPE_RT
      
      RFC1183.
      
   .. data:: RR_TYPE_NSAP
      
      RFC1706.
      
   .. data:: RR_TYPE_NSAP_PTR
      
      RFC1348.
      
   .. data:: RR_TYPE_SIG
      
      2535typecode.
      
   .. data:: RR_TYPE_KEY
      
      2535typecode.
      
   .. data:: RR_TYPE_PX
      
      RFC2163.
      
   .. data:: RR_TYPE_GPOS
      
      RFC1712.
      
   .. data:: RR_TYPE_AAAA
      
      IPv6 address.
      
   .. data:: RR_TYPE_LOC
      
      LOC record  RFC1876.
      
   .. data:: RR_TYPE_NXT
      
      2535typecode.
      
   .. data:: RR_TYPE_EID
      
      draft-ietf-nimrod-dns-01.txt.
      
   .. data:: RR_TYPE_NIMLOC
      
      draft-ietf-nimrod-dns-01.txt.
      
   .. data:: RR_TYPE_SRV
      
      SRV record RFC2782.
      
   .. data:: RR_TYPE_ATMA
   
      http://www.jhsoft.com/rfc/af-saa-0069.000.rtf.
      
   .. data:: RR_TYPE_NAPTR
      
      RFC2915.
      
   .. data:: RR_TYPE_KX
      
      RFC2230.
      
   .. data:: RR_TYPE_CERT
      
      RFC2538.
      
   .. data:: RR_TYPE_A6
      
      RFC2874.
      
   .. data:: RR_TYPE_DNAME
      
      RFC2672.
      
   .. data:: RR_TYPE_SINK
      
      dnsind-kitchen-sink-02.txt.
      
   .. data:: RR_TYPE_OPT
      
      Pseudo OPT record.
      
   .. data:: RR_TYPE_APL
      
      RFC3123.
      
   .. data:: RR_TYPE_DS
      
      draft-ietf-dnsext-delegation.
      
   .. data:: RR_TYPE_SSHFP
      
      SSH Key Fingerprint.
   
   .. data:: RR_TYPE_IPSECKEY
      
      draft-richardson-ipseckey-rr-11.txt.
      
   .. data:: RR_TYPE_RRSIG
      
      draft-ietf-dnsext-dnssec-25.
      
   .. data:: RR_TYPE_NSEC      
   .. data:: RR_TYPE_DNSKEY
   .. data:: RR_TYPE_DHCID
   .. data:: RR_TYPE_NSEC3
   .. data:: RR_TYPE_NSEC3PARAMS
   .. data:: RR_TYPE_UINFO
   .. data:: RR_TYPE_UID
   .. data:: RR_TYPE_GID
   .. data:: RR_TYPE_UNSPEC
   .. data:: RR_TYPE_TSIG
   .. data:: RR_TYPE_IXFR
   .. data:: RR_TYPE_AXFR
   .. data:: RR_TYPE_MAILB
      
      A request for mailbox-related records (MB, MG or MR).
      
   .. data:: RR_TYPE_MAILA
      
      A request for mail agent RRs (Obsolete - see MX).
      
   .. data:: RR_TYPE_ANY
      
      Any type *(wildcard)*.
   
   .. data:: RR_TYPE_DLV
      
      RFC 4431, 5074, DNSSEC Lookaside Validation.
   
Return codes
~~~~~~~~~~~~

Return codes for packets.

.. data:: RCODE_NOERROR
.. data:: RCODE_FORMERR
.. data:: RCODE_SERVFAIL
.. data:: RCODE_NXDOMAIN
.. data:: RCODE_NOTIMPL
.. data:: RCODE_REFUSED
.. data:: RCODE_YXDOMAIN
.. data:: RCODE_YXRRSET
.. data:: RCODE_NXRRSET
.. data:: RCODE_NOTAUTH
.. data:: RCODE_NOTZONE
   
Packet data
~~~~~~~~~~~~

.. data:: PKT_QR

   Query - query flag.
   
.. data:: PKT_AA

   Authoritative Answer - server flag.
   
.. data:: PKT_TC
   
   Truncated - server flag.
   
.. data:: PKT_RD
   
   Recursion desired - query flag.
   
.. data:: PKT_CD

   Checking disabled - query flag.
   
.. data:: PKT_RA
   
   Recursion available - server flag.
   
.. data:: PKT_AD
   
   Authenticated data - server flag.


Verbosity value
~~~~~~~~~~~~~~~~

.. data:: NO_VERBOSE

   No verbose messages.
   
.. data:: VERB_OPS

   Operational information.
   
.. data:: VERB_DETAIL

   Detailed information.
   
.. data:: VERB_QUERY

   Query level information.
   
.. data:: VERB_ALGO

   Algorithm level information.
