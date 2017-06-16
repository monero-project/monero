Introduction
============

Unbound
-------

`Unbound`_  is  an implementation of a DNS resolver, that performs caching and
DNSSEC validation.
Together with unbound, the libunbound library is provided.
This library can be used to convert hostnames to ip addresses, and back, as
well as obtain other information.
Since the resolver allows to specify the class and type of a query (A record,
NS, MX, ...), this library offers powerful resolving tool.
The library also performs public-key validation of results with DNSSEC.

.. _Unbound: http://www.unbound.net/documentation

pyUnbound
---------

The pyUnbound is an extension module for Python which provides an
object-oriented interface to libunbound. 
It is the first Python module which offers thread-safe caching resolver.

The interface was designed with the emphasis on the simplicity of use.
There are two main classes :class:`unbound.ub_ctx` (a validation and resolution
context) and :class:`unbound.ub_result` which contains the validation and
resolution results.
The objects are thread-safe, and a context can be used in non-threaded as well
as threaded environment.
Resolution can be performed blocking and non-blocking (i.e. asynchronous).
The asynchronous method returns from the call immediately, so that processing
can go on, while the results become available later.

Features
--------

* Customizable caching validation resolver for synchronous and asynchronous
  lookups
* Easy to use object interface
* Easy to integrate extension module
* Designed for thread environment (i.e. thread-safe)
* Allows define and customize of local zone and its RR's during the operation
  (i.e. without restart)
* Includes encoding functions to simplify the results retrieval
* Internationalized domain name (`IDN`_) support

.. _IDN: http://en.wikipedia.org/wiki/Internationalized_domain_name

Application area
----------------

* DNS-based applications performing DNS lookups; the caching resolver can
  reduce overhead
* Applications where the validation of DNS records is required
* Great solution for customizable and dynamic DNS-based white/blacklists (spam
  rejection, connection rejection, ...) using the dynamic local zone
  manipulation
