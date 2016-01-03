Configuration interface
=======================

Currently passed to Python module in init(module_id, cfg).

config_file
--------------------

.. class:: config_file

   This class provides these data attributes:

   .. attribute:: verbosity
   
      Verbosity level as specified in the config file.

   .. attribute:: stat_interval
   
      Statistics interval (in seconds).
   
   .. attribute:: stat_cumulative
   
      If false, statistics values are reset after printing them.
   
   .. attribute:: stat_extended
   
      If true, the statistics are kept in greater detail.

   .. attribute:: num_threads
   
      Number of threads to create.

   .. attribute:: port
   
      Port on which queries are answered.

   .. attribute:: do_ip4
   
      Do ip4 query support.

   .. attribute:: do_ip6
   
      Do ip6 query support.

   .. attribute:: do_udp
   
      Do udp query support.
   
   .. attribute:: do_tcp
   
      Do tcp query support.

   .. attribute:: outgoing_num_ports
   
      Outgoing port range number of ports (per thread).

   .. attribute:: outgoing_num_tcp
   
      Number of outgoing tcp buffers per (per thread).

   .. attribute:: incoming_num_tcp
   
      Number of incoming tcp buffers per (per thread).

   .. attribute:: outgoing_avail_ports
   
      Allowed udp port numbers, array with 0 if not allowed.

   .. attribute:: msg_buffer_size
   
      Number of bytes buffer size for DNS messages.

   .. attribute:: msg_cache_size
   
      Size of the message cache.
   
   .. attribute:: msg_cache_slabs
   
      Slabs in the message cache.
   
   .. attribute:: num_queries_per_thread
   
      Number of queries every thread can service.
   
   .. attribute:: jostle_time
   
      Number of msec to wait before items can be jostled out.
   
   .. attribute:: rrset_cache_size
   
      Size of the rrset cache.
   
   .. attribute:: rrset_cache_slabs
   
      Slabs in the rrset cache.
   
   .. attribute:: host_ttl
   
      Host cache ttl in seconds.

   .. attribute:: lame_ttl
   
      Host is lame for a zone ttl, in seconds.

   .. attribute:: infra_cache_slabs
   
      Number of slabs in the infra host cache.
   
   .. attribute:: infra_cache_numhosts
   
      Max number of hosts in the infra cache.
   
   .. attribute:: infra_cache_lame_size
   
      Max size of lame zones per host in the infra cache.

   .. attribute:: target_fetch_policy
   
      The target fetch policy for the iterator.

   .. attribute:: if_automatic
   
      Automatic interface for incoming messages. Uses ipv6 remapping,
      and recvmsg/sendmsg ancillary data to detect interfaces, boolean.
   
   .. attribute:: num_ifs
   
      Number of interfaces to open. If 0 default all interfaces.
   
   .. attribute:: ifs
   
      Interface description strings (IP addresses).

   .. attribute:: num_out_ifs
   
      Number of outgoing interfaces to open. 
      If 0 default all interfaces.

   .. attribute:: out_ifs
   
      Outgoing interface description strings (IP addresses).
      
   .. attribute:: root_hints
   
      The root hints.
   
   .. attribute:: stubs
   
      The stub definitions, linked list.
   
   .. attribute:: forwards
   
      The forward zone definitions, linked list.
   
   .. attribute:: donotqueryaddrs
   
      List of donotquery addresses, linked list.
   
   .. attribute:: acls
   
      List of access control entries, linked list.
   
   .. attribute:: donotquery_localhost
   
      Use default localhost donotqueryaddr entries.

   .. attribute:: harden_short_bufsize
   
      Harden against very small edns buffer sizes.
   
   .. attribute:: harden_large_queries
   
      Harden against very large query sizes.
   
   .. attribute:: harden_glue
   
      Harden against spoofed glue (out of zone data).
   
   .. attribute:: harden_dnssec_stripped
   
      Harden against receiving no DNSSEC data for trust anchor.
   
   .. attribute:: harden_referral_path
   
      Harden the referral path, query for NS,A,AAAA and validate.
   
   .. attribute:: use_caps_bits_for_id
   
      Use 0x20 bits in query as random ID bits.
   
   .. attribute:: private_address
   
      Strip away these private addrs from answers, no DNS Rebinding.
   
   .. attribute:: private_domain
   
      Allow domain (and subdomains) to use private address space.
   
   .. attribute:: unwanted_threshold
   
      What threshold for unwanted action.

   .. attribute:: chrootdir
   
      Chrootdir, if not "" or chroot will be done.
   
   .. attribute:: username
   
      Username to change to, if not "".
   
   .. attribute:: directory
   
      Working directory.
   
   .. attribute:: logfile
   
      Filename to log to.
   
   .. attribute:: pidfile
   
      Pidfile to write pid to.

   .. attribute:: use_syslog
   
      Should log messages be sent to syslogd.

   .. attribute:: hide_identity
   
      Do not report identity (id.server, hostname.bind).
   
   .. attribute:: hide_version
   
      Do not report version (version.server, version.bind).
   
   .. attribute:: identity
   
      Identity, hostname is returned if "".
   
   .. attribute:: version
   
      Version, package version returned if "".

   .. attribute:: module_conf
   
      The module configuration string.
   
   .. attribute:: trust_anchor_file_list
   
      Files with trusted DS and DNSKEYs in zonefile format, list.
   
   .. attribute:: trust_anchor_list
   
      List of trustanchor keys, linked list.
   
   .. attribute:: trusted_keys_file_list
   
      Files with trusted DNSKEYs in named.conf format, list.
   
   .. attribute:: dlv_anchor_file
   
      DLV anchor file.
   
   .. attribute:: dlv_anchor_list
   
      DLV anchor inline.

   .. attribute:: max_ttl
   
      The number of seconds maximal TTL used for RRsets and messages.
   
   .. attribute:: val_date_override
   
      If not 0, this value is the validation date for RRSIGs.
   
   .. attribute:: bogus_ttl 
   
      This value sets the number of seconds before revalidating bogus.
   
   .. attribute:: val_clean_additional
   
      Should validator clean additional section for secure msgs.
   
   .. attribute:: val_permissive_mode
   
      Should validator allow bogus messages to go through.
   
   .. attribute:: val_nsec3_key_iterations
   
      Nsec3 maximum iterations per key size, string.
   
   .. attribute:: key_cache_size
   
      Size of the key cache.
   
   .. attribute:: key_cache_slabs
   
      Slabs in the key cache.
   
   .. attribute:: neg_cache_size
   
      Size of the neg cache.

   
   .. attribute:: local_zones
   
      Local zones config.
   
   .. attribute:: local_zones_nodefault
   
      Local zones nodefault list.
   
   .. attribute:: local_data
   
      Local data RRs configured.

   .. attribute:: remote_control_enable
   
      Remote control section. enable toggle.
   
   .. attribute:: control_ifs
   
      The interfaces the remote control should listen on.
   
   .. attribute:: control_port
   
      Port number for the control port.
   
   .. attribute:: server_key_file
   
      Private key file for server.
   
   .. attribute:: server_cert_file
   
      Certificate file for server.
   
   .. attribute:: control_key_file
   
      Private key file for unbound-control.
   
   .. attribute:: control_cert_file
   
      Certificate file for unbound-control.

   .. attribute:: do_daemonize
   
      Daemonize, i.e. fork into the background.

   .. attribute:: python_script
   
      Python script file.
