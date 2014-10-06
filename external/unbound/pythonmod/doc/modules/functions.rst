Scriptable functions
====================

Network
-------

.. function:: ntohs(netshort)

   This subroutine converts values between the host and network byte order. 
   Specifically, **ntohs()** converts 16-bit quantities from network byte order to host byte order.
   
   :param netshort: 16-bit short addr
   :rtype: converted addr
   
   
Cache
-----

.. function:: storeQueryInCache(qstate, qinfo, msgrep, is_referral)

   Store pending query in local cache.
   
   :param qstate: :class:`module_qstate`
   :param qinfo: :class:`query_info`
   :param msgrep: :class:`reply_info`
   :param is_referal: integer
   :rtype: boolean
   
.. function:: invalidateQueryInCache(qstate, qinfo)

   Invalidate record in local cache.

   :param qstate: :class:`module_qstate`
   :param qinfo: :class:`query_info`


Logging
-------

.. function:: verbose(level, msg)

   Log a verbose message, pass the level for this message.
   No trailing newline is needed.

   :param level: verbosity level for this message, compared to global verbosity setting.
   :param msg: string message

.. function:: log_info(msg)

   Log informational message. No trailing newline is needed.

   :param msg: string message

.. function:: log_err(msg)

   Log error message. No trailing newline is needed.

   :param msg: string message

.. function:: log_warn(msg)

   Log warning message. No trailing newline is needed.

   :param msg: string message

.. function:: log_hex(msg, data, length)

   Log a hex-string to the log. Can be any length.
   performs mallocs to do so, slow. But debug useful.

   :param msg: string desc to accompany the hexdump.
   :param data: data to dump in hex format.
   :param length: length of data.
   
.. function:: log_dns_msg(str, qinfo, reply)

   Log DNS message.
   
   :param str: string message
   :param qinfo: :class:`query_info`
   :param reply: :class:`reply_info`
   
.. function:: log_query_info(verbosity_value, str, qinf)

   Log query information.
   
   :param verbosity_value: see constants
   :param str: string message
   :param qinf: :class:`query_info`
   
.. function:: regional_log_stats(r)

   Log regional statistics.
   
   :param r: :class:`regional`

Debugging
---------

.. function:: strextstate(module_ext_state)

   Debug utility, module external qstate to string.
   
   :param module_ext_state: the state value.
   :rtype: descriptive string.

.. function:: strmodulevent(module_event)

   Debug utility, module event to string.
   
   :param module_event: the module event value.
   :rtype: descriptive string.
   
.. function:: ldns_rr_type2str(atype)

   Convert RR type to string.
   
.. function:: ldns_rr_class2str(aclass)

   Convert RR class to string.
