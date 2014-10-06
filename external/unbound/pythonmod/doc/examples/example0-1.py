
def init(id, cfg):
   log_info("pythonmod: init called, module id is %d port: %d script: %s" % (id, cfg.port, cfg.python_script))
   return True

def deinit(id):
   log_info("pythonmod: deinit called, module id is %d" % id)
   return True

def inform_super(id, qstate, superqstate, qdata):
   return True

def operate(id, event, qstate, qdata):
   log_info("pythonmod: operate called, id: %d, event:%s" % (id, strmodulevent(event)))

   if event == MODULE_EVENT_NEW:
      qstate.ext_state[id] = MODULE_WAIT_MODULE 
      return True

   if event == MODULE_EVENT_MODDONE:
      log_info("pythonmod: module we are waiting for is done")
      qstate.ext_state[id] = MODULE_FINISHED 
      return True

   if event == MODULE_EVENT_PASS:
      log_info("pythonmod: event_pass")
      qstate.ext_state[id] = MODULE_ERROR 
      return True

   log_err("pythonmod: BAD event")
   qstate.ext_state[id] = MODULE_ERROR
   return True

log_info("pythonmod: script loaded.")
