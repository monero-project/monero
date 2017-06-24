Scriptable functions
====================

Network
-------

.. function:: ntohs(netshort)

   This subroutine converts values between the host and network byte order. 
   Specifically, **ntohs()** converts 16-bit quantities from network byte order
   to host byte order.

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


EDNS options
------------

.. function:: register_edns_option(env, code, bypass_cache_stage=False, no_aggregation=False)

    Register EDNS option code.

    :param env: :class:`module_env`
    :param code: option code(integer)
    :param bypass_cache_stage: whether to bypass the cache response stage
    :param no_aggregation: whether this query should be unique
    :return: ``1`` if successful, ``0`` otherwise
    :rtype: integer

.. function:: edns_opt_list_find(list, code)

    Find the EDNS option code in the EDNS option list.

    :param list: linked list of :class:`edns_option`
    :param code: option code (integer)
    :return: the edns option if found or None
    :rtype: :class:`edns_option` or None

.. function:: edns_opt_list_remove(list, code);

    Remove an ENDS option code from the list.
    .. note:: All :class:`edns_option` with the code will be removed

    :param list: linked list of :class:`edns_option`
    :param code: option code (integer)
    :return: ``1`` if at least one :class:`edns_option` was removed, ``0`` otherwise
    :rtype: integer

.. function:: edns_opt_list_append(list, code, data, region)

    Append given EDNS option code with data to the list.

    :param list: linked list of :class:`edns_option`
    :param code: option code (integer)
    :param data: EDNS data. **Must** be a :class:`bytearray`
    :param region: :class:`regional`

.. function:: edns_opt_list_is_empty(list)

    Check if an EDNS option list is empty.

    :param list: linked list of :class:`edns_option`
    :return: ``1`` if list is empty, ``0`` otherwise
    :rtype: integer


Inplace callbacks
-----------------

.. function:: inplace_cb_reply(qinfo, qstate, rep, rcode, edns, opt_list_out, region)

    Function prototype for callback functions used in
    `register_inplace_cb_reply`_, `register_inplace_cb_reply_cache`_,
    `register_inplace_cb_reply_local` and `register_inplace_cb_reply_servfail`.

    :param qinfo: :class:`query_info`
    :param qstate: :class:`module_qstate`
    :param rep: :class:`reply_info`
    :param rcode: return code (integer), check ``RCODE_`` constants.
    :param edns: :class:`edns_data`
    :param opt_list_out: :class:`edns_option`. EDNS option list to append options to.
    :param region: :class:`regional`

.. function:: register_inplace_cb_reply(py_cb, env)

    Register py_cb as an inplace reply callback function.

    :param py_cb: Python function that follows `inplace_cb_reply`_'s prototype. **Must** be callable.
    :param env: :class:`module_env`
    :return: True on success, False otherwise
    :rtype: boolean

.. function:: register_inplace_cb_reply_cache(py_cb, env)

    Register py_cb as an inplace reply_cache callback function.

    :param py_cb: Python function that follows `inplace_cb_reply`_'s prototype. **Must** be callable.
    :param env: :class:`module_env`
    :return: True on success, False otherwise
    :rtype: boolean

.. function:: register_inplace_cb_reply_local(py_cb, env)

    Register py_cb as an inplace reply_local callback function.

    :param py_cb: Python function that follows `inplace_cb_reply`_'s prototype. **Must** be callable.
    :param env: :class:`module_env`
    :return: True on success, False otherwise
    :rtype: boolean

.. function:: register_inplace_cb_reply_servfail(py_cb, env)

    Register py_cb as an inplace reply_servfail callback function.

    :param py_cb: Python function that follows `inplace_cb_reply`_'s prototype. **Must** be callable.
    :param env: :class:`module_env`
    :return: True on success, False otherwise
    :rtype: boolean


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
