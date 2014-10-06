Unbound module documentation
================================

.. automodule:: unbound

Class ub_ctx
--------------
.. autoclass:: ub_ctx
	:members:
	:undoc-members:
	
	.. automethod:: __init__ 

Class ub_result
----------------------
.. autoclass:: ub_result
	:members:

	.. attribute:: qname

		The original question, name text string.

	.. attribute:: qtype

		The class asked for.

	.. attribute:: canonname

		Canonical name for the result (the final cname). May be empty if no canonical name exists.

	.. attribute:: answer_packet
		
		The DNS answer packet. Network formatted. Can contain DNSSEC types.

	.. attribute:: havedata
		
		If there is any data, this property is true. If false, there was no data (nxdomain may be true, rcode can be set).

	.. attribute:: secure
		
		True, if the result is validated securely.
	 	False, if validation failed or domain queried has no security info.
	 
	  	It is possible to get a result with no data (havedata is false),
	  	and secure is true. This means that the non-existance of the data
	  	was cryptographically proven (with signatures).

	.. attribute:: bogus
		
		If the result was not secure (secure==0), and this result is due  to a security failure, bogus is true.
	 	This means the data has been actively tampered with, signatures
	 	failed, expected signatures were not present, timestamps on 
	 	signatures were out of date and so on.
	 
	 	If secure==0 and bogus==0, this can happen if the data is not secure 
	 	because security is disabled for that domain name. 
	 	This means the data is from a domain where data is not signed.

	.. attribute:: nxdomain

		If there was no data, and the domain did not exist, this is true. 
		If it is false, and there was no data, then the domain name is purported to exist, but the requested data type is not available.

	.. attribute:: rcode
		
		DNS RCODE for the result. May contain additional error code if there was no data due to an error. 
		0 (RCODE_NOERROR) if okay. See predefined `RCODE_` constants.
	
		RCODE can be represented in display representation form (string) using :attr:`rcode_str` attribute.

Class ub_data
----------------------
.. autoclass:: ub_data
	:members:

Functions
----------------------
.. autofunction:: reverse
.. autofunction:: idn2dname
.. autofunction:: dname2idn

Predefined constants
-----------------------

**RCODE**
	* RCODE_FORMERR = 1
	* RCODE_NOERROR = 0
	* RCODE_NOTAUTH = 9
	* RCODE_NOTIMPL = 4
	* RCODE_NOTZONE = 10
	* RCODE_NXDOMAIN = 3
	* RCODE_NXRRSET = 8
	* RCODE_REFUSED = 5
	* RCODE_SERVFAIL = 2
	* RCODE_YXDOMAIN = 6
	* RCODE_YXRRSET = 7

**RR_CLASS**
	* RR_CLASS_ANY = 255
	* RR_CLASS_CH = 3
	* RR_CLASS_HS = 4
	* RR_CLASS_IN = 1
	* RR_CLASS_NONE = 254

**RR_TYPE**
	* RR_TYPE_A = 1
	* RR_TYPE_A6 = 38
	* RR_TYPE_AAAA = 28
	* RR_TYPE_AFSDB = 18
	* RR_TYPE_ANY = 255
	* RR_TYPE_APL = 42
	* RR_TYPE_ATMA = 34
	* RR_TYPE_AXFR = 252
	* RR_TYPE_CERT = 37
	* RR_TYPE_CNAME = 5
	* RR_TYPE_DHCID = 49
	* RR_TYPE_DLV = 32769
	* RR_TYPE_DNAME = 39
	* RR_TYPE_DNSKEY = 48
	* RR_TYPE_DS = 43
	* RR_TYPE_EID = 31
	* RR_TYPE_GID = 102
	* RR_TYPE_GPOS = 27
	* RR_TYPE_HINFO = 13
	* RR_TYPE_IPSECKEY = 45
	* RR_TYPE_ISDN = 20
	* RR_TYPE_IXFR = 251
	* RR_TYPE_KEY = 25
	* RR_TYPE_KX = 36
	* RR_TYPE_LOC = 29
	* RR_TYPE_MAILA = 254
	* RR_TYPE_MAILB = 253
	* RR_TYPE_MB = 7
	* RR_TYPE_MD = 3
	* RR_TYPE_MF = 4
	* RR_TYPE_MG = 8
	* RR_TYPE_MINFO = 14
	* RR_TYPE_MR = 9
	* RR_TYPE_MX = 15
	* RR_TYPE_NAPTR = 35
	* RR_TYPE_NIMLOC = 32
	* RR_TYPE_NS = 2
	* RR_TYPE_NSAP = 22
	* RR_TYPE_NSAP_PTR = 23
	* RR_TYPE_NSEC = 47
	* RR_TYPE_NSEC3 = 50
	* RR_TYPE_NSEC3PARAMS = 51
	* RR_TYPE_NULL = 10
	* RR_TYPE_NXT = 30
	* RR_TYPE_OPT = 41
	* RR_TYPE_PTR = 12
	* RR_TYPE_PX = 26
	* RR_TYPE_RP = 17
	* RR_TYPE_RRSIG = 46
	* RR_TYPE_RT = 21
	* RR_TYPE_SIG = 24
	* RR_TYPE_SINK = 40
	* RR_TYPE_SOA = 6
	* RR_TYPE_SRV = 33
	* RR_TYPE_SSHFP = 44
	* RR_TYPE_TSIG = 250
	* RR_TYPE_TXT = 16
	* RR_TYPE_UID = 101
	* RR_TYPE_UINFO = 100
	* RR_TYPE_UNSPEC = 103
	* RR_TYPE_WKS = 11
	* RR_TYPE_X25 = 19
