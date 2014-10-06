.. _example_handler:

Fundamentals
================

This basic example shows how to create simple python module which will pass on the requests to the iterator.

How to enable python module
----------------------------
If you look into unbound configuration file, you can find the option `module-config` which specifies the names and the order of modules to be used.
Example configuration::

	module-config: "validator python iterator"

As soon as the DNS query arrives, Unbound calls modules starting from leftmost - the validator *(it is the first module on the list)*.
The validator does not know the answer *(it can only validate)*, thus it will pass on the event to the next module.
Next module is python which can

	a) generate answer *(response)*
		When python module generates the response unbound calls validator. Validator grabs the answer and determines the security flag.

	b) pass on the event to the iterator.
		When iterator resolves the query, Unbound informs python module (event :data:`module_event_moddone`). In the end, when the python module is done, validator is called.

Note that the python module is called with :data:`module_event_pass` event, because new DNS event was already handled by validator.

Another situation occurs when we use the following configuration::

	module-config: "python validator iterator"

Python module is the first module here, so it's invoked with :data:`module_event_new` event *(new query)*.

On Python module initialization, module loads script from `python-script` option::

	python-script: "/unbound/test/ubmodule.py"

Simple python module step by step
---------------------------------

Script file must contain four compulsory functions:

.. function:: init(id, cfg)

   Initialize module internals, like database etc.
   Called just once on module load.

   :param id: module identifier (integer)
   :param cfg: :class:`config_file` configuration structure

::

   def init(id, cfg):
      log_info("pythonmod: init called, module id is %d port: %d script: %s" % (id, cfg.port, cfg.python_script))
      return True


.. function:: deinit(id)

   Deinitialize module internals.
   Called just once on module unload.

   :param id: module identifier (integer)

::

   def deinit(id):
      log_info("pythonmod: deinit called, module id is %d" % id)
      return True


.. function:: inform_super(id, qstate, superqstate, qdata)

   Inform super querystate about the results from this subquerystate.
   Is called when the querystate is finished.

   :param id: module identifier (integer)
   :param qstate: :class:`module_qstate` Query state
   :param superqstate: :class:`pythonmod_qstate` Mesh state
   :param qdata: :class:`query_info` Query data

::

   def inform_super(id, qstate, superqstate, qdata):
      return True



.. function:: operate(id, event, qstate, qdata)

   Perform action on pending query. Accepts a new query, or work on pending query.

   You have to set qstate.ext_state on exit.
   The state informs unbound about result and controls the following states.

   :param id: module identifier (integer)
   :param qstate: :class:`module_qstate` query state structure
   :param qdata: :class:`query_info` per query data, here you can store your own data

::

   def operate(id, event, qstate, qdata):
      log_info("pythonmod: operate called, id: %d, event:%s" % (id, strmodulevent(event)))
      if event == MODULE_EVENT_NEW:
         qstate.ext_state[id] = MODULE_WAIT_MODULE 
         return True

      if event == MODULE_EVENT_MODDONE:
         qstate.ext_state[id] = MODULE_FINISHED 
         return True

      if event == MODULE_EVENT_PASS:
         qstate.ext_state[id] = MODULE_ERROR 
         return True

      log_err("pythonmod: BAD event")
      qstate.ext_state[id] = MODULE_ERROR
      return True


Complete source code
--------------------

..	literalinclude:: example0-1.py
	:language: python

As you can see, the source code is much more flexible in contrast to C modules. 
Moreover, compulsory functions called on appropriate module events allows to handle almost
anything from web control to query analysis.

